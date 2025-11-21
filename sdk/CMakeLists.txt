add_library(switch_sdk
    StateDB.cpp
    AgentBase.cpp
)

target_include_directories(switch_sdk
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(switch_sdk
    PUBLIC
        redis++::redis++
        nlohmann_json::nlohmann_json
)