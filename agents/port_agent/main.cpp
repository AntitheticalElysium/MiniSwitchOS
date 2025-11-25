#include <AgentBase.hpp>

#include <iostream>

using namespace sdk;

struct InterfaceState {
    std::string admin_config = "down"; // Default to safe state
    std::string hal_status   = "down";
    std::string computed_oper = "down";
};

class PortAgent : public AgentBase {
    public:
        PortAgent() : AgentBase("PortAgent") {}

    protected:
        std::map<std::string, InterfaceState> port_cache_;

        void initial_sync() override {
            std::cout << "[PortAgent] Loading Config and HAL state..." << std::endl;

            auto config_keys = stateDB().scanKeys("sysdb/interface/*/admin_status/config");
            for (const auto& key: config_keys) {
                auto val = stateDB().get(key);
                if (val) update_cache_from_key(key, *val);
            }

            auto hal_keys = stateDB().scanKeys("sysdb/hal/port/*/oper_status/status");
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
                std::string id = extract_id(key);
                reconcile(id);
            }
        }
    
    private:
        void reconcile(const std::string& intf_id) {
            auto& state = port_cache_[intf_id];

            std::string new_oper = "down";
            if (state.admin_config == "up" && state.hal_status == "up") new_oper = "up";

            // Only write if changed
            if (state.computed_oper != new_oper) {
                state.computed_oper = new_oper;
            
                std::string key = statusPath("interface/" + intf_id, "oper_status");
                json payload = {
                    {"value", new_oper},
                    {"reason", (new_oper == "down" && state.admin_config == "down") ? "admin_down" : "link_failure"},
                    {"ts", std::time(nullptr)}
                };

                std::cout << "[PortAgent] Reconciled " << intf_id << ": " << new_oper << std::endl;
                stateDB().set(key, payload);
            }

        }

        void reconcile_all() {
            for (auto const& [id, state] : port_cache_) {
                reconcile(id);
            }
        }

        bool update_cache_from_key(const std::string& key, const json& value) {
            // sysdb/interface/<ID>/admin_status/config
            if (key.find("/admin_status/config") != std::string::npos) {
                std::string id = extract_id(key);
                port_cache_[id].admin_config = value.value("admin_status", "down");
                return true;
            }

            // sysdb/hal/port/<ID>/oper_status/status
            if (key.find("/hal/port/") != std::string::npos &&
                key.find("/oper_status/status") != std::string::npos) {
                std::string id = extract_id_hal(key);
                port_cache_[id].hal_status = value.value("link_status", "down");
                return true;
            }

            return false;
        }

        std::string extract_id(const std::string& key) {
            // extract "eth1" from sysdb/interface/eth1/...
            size_t start = key.find("/interface/") + 11;
            size_t end = key.find("/", start);
            return key.substr(start, end - start);
        }

        std::string extract_id_hal(const std::string& key) {
            // extract "1" from sysdb/hal/port/1/...
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