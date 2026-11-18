#include "ipc_server.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace IPC {

Server& Server::instance() {
    static Server inst;
    return inst;
}

Server::Server() = default;

Server::~Server() {
    stop();
}

std::string Server::get_socket_path() {
    const char *runtime_dir = std::getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        return std::string(runtime_dir) + "/voxboard.sock";
    }
    return "/tmp/voxboard.sock";
}

bool Server::send_command(const std::string &cmd) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::string path = get_socket_path();
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return false;
    }

    std::string msg = cmd + "\n";
    write(fd, msg.c_str(), msg.size());

    char buf[16];
    read(fd, buf, sizeof(buf));
    close(fd);
    return true;
}

bool Server::start(std::function<void(const std::string&)> handler) {
    handler_ = handler;
    std::string path = get_socket_path();
    unlink(path.c_str());

    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) return false;

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    if (listen(server_fd_, 8) < 0) {
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }

    running_ = true;
    thread_ = std::thread(&Server::loop, this);
    return true;
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        // Connect to unblock accept
        int dummy = socket(AF_UNIX, SOCK_STREAM, 0);
        if (dummy >= 0) {
            struct sockaddr_un addr;
            std::memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            std::string path = get_socket_path();
            std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);
            connect(dummy, (struct sockaddr*)&addr, sizeof(addr));
            close(dummy);
        }
        close(server_fd_);
        server_fd_ = -1;
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    unlink(get_socket_path().c_str());
}

void Server::loop() {
    while (running_) {
        int client_fd = accept(server_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            if (!running_) break;
            continue;
        }

        char buf[256];
        ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::string line(buf);
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                line.pop_back();
            }
            if (handler_ && !line.empty()) {
                handler_(line);
            }
            write(client_fd, "OK\n", 3);
        }
        close(client_fd);
    }
}

} // namespace IPC
