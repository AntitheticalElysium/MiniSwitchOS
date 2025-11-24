#include "AgentBase.hpp"

namespace sdk {

void AgentBase::run() {
    std::cout << "[" << agent_name_ << "] Starting up..." << std::endl;

    // Init DB
    db_ = std::make_unique<StateDB>();
    // Init Sync
    std::cout << "[" << agent_name_ << "] Performing Initial Sync..." << std::endl;
    initial_sync();

    std::cout << "[" << agent_name_ << "] Entering Event Loop..." << std::endl;
    
    // Later sub to specific paths
    db_->subscribe("sysdb/*", 
        [this](std::string key, std::string raw_val_ignored) {
            // Redis notification tells us Key changed 
            auto val = db_->get(key);
            if (val) {
                this->handle_update(key, *val);
            }
        }
    );

}