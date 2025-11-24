#include <AgentBase.hpp>

#include <chrono>
#include <iostream>
#include <iomanip>

using namespace sdk;

class LogAgent : public AgentBase {
    public:
        LogAgent() : AgentBase("LogAgent") {}

    protected:
        void initial_sync() override {
            std::cout << "[LoggingAgent] Initial sync complete (no-op)." << std::endl;
        }

        void handle_update(const std::string& key, const json& value) override {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            
            std::cout << "[EVENT] " << std::put_time(std::localtime(&now_c), "%c")
                      << " | KEY: " << key 
                      << " | VAL: " << value.dump() 
                      << std::endl;
        }
};

int main() {
    LogAgent agent;
    agent.run();
    return 0;
}