add_executable(hal_sim main.cpp HalCore.cpp)

target_link_libraries(hal_sim PRIVATE switch_sdk)