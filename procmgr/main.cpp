#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <filesystem>

struct AgentConfig {
    std::string name;
    std::string binary_path;
    std::vector<char*> args; 
};

// Get the directory where procmgr is located
std::string get_base_dir() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count == -1) {
        return ".";
    }
    std::string path(result, count);
    return std::filesystem::path(path).parent_path().string();
}

std::string BASE_DIR;

std::vector<AgentConfig> registry = {
    //{"log_agent", "../agents/log_agent/log_agent", {}},
    {"hal_sim", "../hal/hal_sim", {}},
    {"port_agent", "../agents/port_agent/port_agent", {}},
};

std::map<pid_t, AgentConfig> pid_to_agent;
volatile sig_atomic_t running = 1;

extern "C" void handle_sigint(int) {
    running = 0;
}

void spawn_agent(const AgentConfig& agent) {
    std::cout << "[ProcMgr] Spawning " << agent.name << "..." << std::endl;
    
    // Build absolute path
    std::string abs_path = BASE_DIR + "/" + agent.binary_path;
    
    pid_t pid = fork();

    if (pid == -1) {
        std::cerr << "[ProcMgr] Failed to fork for " << agent.name << std::endl;
        return;
    } else if (pid == 0) {
        // Child 
        std::vector<char*> args = agent.args;
        args.insert(args.begin(), const_cast<char*>(abs_path.c_str()));
        args.push_back(nullptr); // null terminated 

        execvp(abs_path.c_str(), args.data());
        std::cerr << "[ProcMgr] Failed to exec " << agent.name << " at " << abs_path << std::endl;
        _exit(1);
    } else {
        // Parent process
        pid_to_agent[pid] = agent;
        std::cout << "[ProcMgr] " << agent.name << " started with PID " << pid << std::endl;
    }
}

void terminate_all() {
    std::cout << "[ProcMgr] Terminating all agents..." << std::endl;

    for (const auto& [pid, agent] : pid_to_agent) {
        std::cout << "[ProcMgr] Sending SIGTERM to " << agent.name << " (PID " << pid << ")" << std::endl;
        kill(pid, SIGTERM);
    }
    
    // Graceful shutdown wait
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    
    while (!pid_to_agent.empty() && std::chrono::steady_clock::now() < deadline) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            if (pid_to_agent.count(pid)) {
                std::cout << "[ProcMgr] " << pid_to_agent[pid].name << " exited gracefully" << std::endl;
                pid_to_agent.erase(pid);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Force kill any remaining processes
    if (!pid_to_agent.empty()) {
        std::cout << "[ProcMgr] Sending SIGKILL to remaining agents..." << std::endl;
        for (const auto& [pid, agent] : pid_to_agent) {
            std::cout << "[ProcMgr] Force killing " << agent.name << " (PID " << pid << ")" << std::endl;
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
        pid_to_agent.clear();
    }
    
    std::cout << "[ProcMgr] All agents terminated" << std::endl;
}

int main() {
    std::cout << "[ProcMgr] Starting Process Manager..." << std::endl;
    
    // Determine base directory from executable location
    BASE_DIR = get_base_dir();
    std::cout << "[ProcMgr] Base directory: " << BASE_DIR << std::endl;
    
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);

    for (const auto& agent : registry) {
        spawn_agent(agent);
    }

    while (running) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            if (pid_to_agent.count(pid)) {
                AgentConfig agent = pid_to_agent[pid];
                pid_to_agent.erase(pid);
                
                // Log exit reason
                if (WIFEXITED(status)) {
                    std::cout << "[ProcMgr] " << agent.name << " (PID " << pid 
                              << ") exited with code " << WEXITSTATUS(status) << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cout << "[ProcMgr] " << agent.name << " (PID " << pid 
                              << ") killed by signal " << WTERMSIG(status) << std::endl;
                }

                // Restart 
                std::cout << "[ProcMgr] Restarting " << agent.name << " in 1 second..." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                for (const auto& reg_agent : registry) {
                    if (reg_agent.name == agent.name) {
                        spawn_agent(reg_agent);
                        break;
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    terminate_all();
    return 0;
}