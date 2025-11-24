#pragma once

#include <sw/redis++/redis++.h>
#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <vector>
#include <functional>
#include <iostream>

using json = nlohmann::json;

namespace sdk {

class StateDB {
    public:
        explicit StateDB(const std::string& redis_url = "tcp://127.0.0.1:6379");

        void set(const std::string& key, const json& value);
        std::optional<json> get(const std::string& key);

        // Sync helper (returns all key matching pattern)
        std::vector<std::string> scanKeys(const std::string& pattern);

        // Event loop connector
        using MessageHandler = std::function<void(std::string key, std::string value)>;
        void subscribe(const std::string& pattern, MessageHandler handler);

    private: 
        std::shared_ptr<sw::redis::Redis> redis_;
};

}