#include "StateDB.hpp"

namespace sdk {

StateDB::StateDB(const std::string& redis_url) {
    redis_ = std::make_shared<sw::redis::Redis>(redis_url); 
    // Force enable keyspace notifications
    try {
        redis_->command("config", "set", "notify-keyspace-events", "KEA");
    } catch (const std::exception& e) {
        std::cerr << "[StateDB] Warning: Could not set notify-keyspace-events. "
                  << "Ensure Redis is running. Error: " << e.what() << std::endl;
    }
}

void StateDB::set(const std::string& key, const json& value) {
    redis_->set(key, value.dump());
}

std::optional<json> StateDB::get(const std::string& key) {
    auto val = redis_->get(key);
    if (val) {
        try {
            return json::parse(*val);
        } catch (...) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::vector<std::string> StateDB::scanKeys(const std::string& pattern) {
    std::vector<std::string> keys; 
    sw::redis::Cursor cursor = 0;

    // Count per batch 10
    do {
        cursor = redis_->scan(cursor, pattern, 10, std::back_inserter(keys));
    } while (cursor != 0);

    return keys;
}

void StateDB::subscribe(const std::string& pattern, MessageHandler handler) {
    auto sub = redis_->subscriber();

    sub.on_pmessage([handler](std::string pattern, std::string channel, std::string msg) {
        // channel comes in like: "__keyspace@0__:sysdb/interface/eth0/status"
        // Strip the prefix to get: "sysdb/interface/eth0/status"
        
        std::string prefix = "__keyspace@0__:";
        if (channel.find(prefix) == 0) {
            std::string actual_key = channel.substr(prefix.length());
            // Handler expects (key, event_type)
            handler(actual_key, msg);
        }
    });

    // If agent asks for "sysdb/*", ask Redis for "__keyspace@0__:sysdb/*"
    std::string redis_pattern = "__keyspace@0__:" + pattern;
    sub.psubscribe(redis_pattern);

    while (true) {
        try {
            sub.consume();
        } catch (const sw::redis::TimeoutError &e) {
            continue;
        } catch (const sw::redis::Error &e) {
            std::cerr << "[StateDB] Redis Error: " << e.what() << std::endl;
        }
    }
}

}