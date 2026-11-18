#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace IPC {

class Server {
public:
    static Server& instance();

    Server();
    ~Server();

    static std::string get_socket_path();
    static bool send_command(const std::string &cmd);

    bool start(std::function<void(const std::string&)> handler);
    void stop();

private:
    void loop();

    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::function<void(const std::string&)> handler_;
};

} // namespace IPC
