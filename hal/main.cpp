#include <StateDB.hpp>
#include <iostream>
#include <thread>
#include <chrono>

using namespace sdk;
using json = nlohmann::json;

int main() {
    std::cout << "[HAL] Hardware Simulator Starting..." << std::endl;
    StateDB db; 

    // Simulate Port 1 - hardware layer status
    std::string port1_key = "sysdb/hardware/port/1/status";
    bool link_up = false;

    while (true) {
        // Toggle state 
        link_up = !link_up;
        std::string status = link_up ? "up" : "down";
        int speed = link_up ? 1000 : 0;

        // Create payload 
        json payload = {
            {"linkStatus", status},      
            {"speed", speed},           
            {"lastChanged", std::time(nullptr)}
        };

        // Write to hardware status
        std::cout << "[HAL] Simulating Link " << status << " on Port 1" << std::endl;
        db.set(port1_key, payload);

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    return 0;
}