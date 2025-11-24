#include "StateDB.hpp"

namespace sdk {

StateDB::StateDB(const std::string& redis_url) {
    redis_ = std::make_shared<sw::redis::Redis>(redis_url); 
}

void StateDB::set(const std::string& key, const json& value) {
    redis_->set(key, value.dump());
}

std::optional<json> StateDB::get(const std::string& key) {
    auto val = redis_->get(key);
    if (val) {
        return json::parse(*val);
    }
    return std::nullopt ;
}

std::vector<std::string> StateDB::scanKeys(const std::string& pattern) {
    std::vector<std::string> keys; 
    auto it = redis_->scan(pattern);
    for (auto& key : it) {
        keys.push_back(key);
    }  
    return keys;
}

void StateDB::subscribe(const std::string& pattern, MessageHandler handler) {
    auto sub = redis_->subscriber();
    sub.psubscribe(pattern, [handler](const std::string& chan, const std::string& msg) {
        handler(chan, msg);
    });

    sub.consume();
}

}