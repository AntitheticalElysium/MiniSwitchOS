#include <StateDB.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace sdk;
using json = nlohmann::json;

int main() {
    std::cout << "[HAL] Hardware Simulator Starting..." << std::endl;
    StateDB db; 

    // Simulate Port 1
    std::string port1_key = "sysdb/hal/port/1/oper_status";
    bool link_status = false;

    while (true) {
        // Toggle state 
        link_status = !link_status;
        std::string status = link_status ? "up" : "down";
        int speed = link_status ? 1000 : 0;

        // Create payload 
        json payload = {
            {"link_status", status},
            {"speed", speed},
            {"timestamp", std::time(nullptr)}
        };

        // Write to hardware 
        std::cout << "[HAL] Simulating Link " << status << " on Port 1" << std::endl;
        db.set(port1_key, payload);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}