cmake_minimum_required(VERSION 3.16)
project(MiniSwitchOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# System installed
find_package(redis++ REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Boost REQUIRED)

# Subdirs
add_subdirectory(sdk)
add_subdirectory(hal)
add_subdirectory(procmgr)
add_subdirectory(agents/log_agent)
add_subdirectory(agents/port_agent)