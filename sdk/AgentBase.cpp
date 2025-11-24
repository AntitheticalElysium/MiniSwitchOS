#include "AgentBase.hpp"

namespace sdk {

AgentBase::AgentBase(std::string name) 
    : agent_name_(std::move(name)) {
    db_ = std::make_unique<StateDB>();
}

// Schema: sysdb/<entity>/<param>/config
std::string AgentBase::configPath(const std::string& entity, const std::string& param) {
    return "sysdb/" + entity + "/" + param + "/config";
}

// Schema: sysdb/<entity>/<param>/status
std::string AgentBase::statusPath(const std::string& entity, const std::string& param) {
    return "sysdb/" + entity + "/" + param + "/status";
}


void AgentBase::run() {
    std::cout << "[" << agent_name_ << "] Starting up..." << std::endl;

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

}