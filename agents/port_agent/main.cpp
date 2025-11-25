#include <AgentBase.hpp>

#include <iostream>

using namespace sdk;

struct InterfaceState {
    bool adminEnabled = false;       // Config intent (default: disabled/safe)
    std::string linkStatus = "down"; // Physical link from HAL
    std::string operStatus = "down"; // Computed operational state
};

class PortAgent : public AgentBase {
    public:
        PortAgent() : AgentBase("PortAgent") {}

    protected:
        std::map<std::string, InterfaceState> port_cache_;

        void initial_sync() override {
            std::cout << "[PortAgent] Loading Config and HAL state..." << std::endl;

            // Scan for interface admin config
            auto config_keys = stateDB().scanKeys("sysdb/interface/*/config");
            for (const auto& key: config_keys) {
                auto val = stateDB().get(key);
                if (val) update_cache_from_key(key, *val);
            }

            // Scan for hardware port status
            auto hal_keys = stateDB().scanKeys("sysdb/hardware/port/*/status");
            for (const auto& key: hal_keys) {
                auto val = stateDB().get(key);
                if (val) update_cache_from_key(key, *val);
            }

            // Reconcile all interfaces
            reconcile_all();
        }

        void handle_update(const std::string& key, const json& value) override {
            // Only update cache if relevant
            if (update_cache_from_key(key, value)) {
                // Extract correct ID based on key type
                std::string id;
                if (key.find("/hardware/port/") != std::string::npos) {
                    id = extract_id_hal(key);
                } else {
                    id = extract_id(key);
                }
                reconcile(id);
            }
        }
    
    private:
        void reconcile(const std::string& intf_id) {
            auto& state = port_cache_[intf_id];

            std::string new_oper = "down";
            if (state.adminEnabled && state.linkStatus == "up") {
                new_oper = "up";
            }

            // Only write if changed
            if (state.operStatus != new_oper) {
                state.operStatus = new_oper;
            
                // Write to sysdb/interface/<id>/status
                std::string key = "sysdb/interface/" + intf_id + "/status";
                
                // Determine reason for down state
                std::string reason = "";
                if (new_oper == "down") {
                    reason = !state.adminEnabled ? "adminDisabled" : "linkDown";
                }

                json payload = {
                    {"operStatus", new_oper},
                    {"linkStatus", state.linkStatus},
                    {"adminEnabled", state.adminEnabled},
                    {"reason", reason},
                    {"lastChanged", std::time(nullptr)}
                };

                std::cout << "[PortAgent] Reconciled " << intf_id << ": operStatus=" << new_oper << std::endl;
                stateDB().set(key, payload);
            }

        }

        void reconcile_all() {
            for (auto const& [id, state] : port_cache_) {
                reconcile(id);
            }
        }

        bool update_cache_from_key(const std::string& key, const json& value) {
            // sysdb/interface/<ID>/config - admin configuration
            if (key.find("/interface/") != std::string::npos &&
                key.find("/config") != std::string::npos) {
                std::string id = extract_id(key);
                port_cache_[id].adminEnabled = value.value("adminEnabled", false);
                return true;
            }

            // sysdb/hardware/port/<ID>/status - HAL link state
            if (key.find("/hardware/port/") != std::string::npos &&
                key.find("/status") != std::string::npos) {
                std::string id = extract_id_hal(key);
                port_cache_[id].linkStatus = value.value("linkStatus", "down");
                return true;
            }

            return false;
        }

        std::string extract_id(const std::string& key) {
            // extract "eth1" from sysdb/interface/eth1/config
            size_t start = key.find("/interface/") + 11;
            size_t end = key.find("/", start);
            return key.substr(start, end - start);
        }

        std::string extract_id_hal(const std::string& key) {
            // extract "1" from sysdb/hardware/port/1/status
            size_t start = key.find("/port/") + 6;
            size_t end = key.find("/", start);
            return key.substr(start, end - start);
        } 
};

int main() {
    PortAgent agent;
    agent.run();
    return 0;
}