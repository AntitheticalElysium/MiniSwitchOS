#pragma once 

#include <string>
#include <iostream>
#include "StateDB.hpp"

namespace sdk {

class AgentBase {
    public:
        AgentBase(std::string name);
        virtual ~AgentBase() = default;

        void run();

    protected:
        // Scan DB and populate initial state
        virtual void initial_sync() = 0;

        // Called whenever watched key is updated
        virtual void handle_update(const std::string& key, const json& value) = 0;

        // Standardized paths
        std::string configPath(const std::string& entity, const std::string& param);
        std::string statusPath(const std::string& entity, const std::string& param);

        StateDB& stateDB() { return *db_; }

    private:
        std::string agent_name_;
        std::unique_ptr<StateDB> db_;
};

}