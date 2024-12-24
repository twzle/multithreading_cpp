#pragma once

#include "server.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <cassert>

namespace matrix_service {

class MtBlockingServer : public Server {
public:
    explicit MtBlockingServer(Config conf);
    MtBlockingServer(const MtBlockingServer&) = delete;
    MtBlockingServer& operator=(const MtBlockingServer&) = delete;

    MtBlockingServer(MtBlockingServer&& another)
        : Server(std::move(another))
    {
        Swap(another);
    }
    MtBlockingServer& operator=(MtBlockingServer&& another)
    {
        static_cast<Server&>(*this) = std::move(another);
        Swap(another);
        return *this;
    }
    ~MtBlockingServer();

    void Run() override;
    void OnStop() override;

private:
    void Swap(MtBlockingServer& another);

    template<typename IOFunc>
    bool TryIOEnough(int client_socket, std::size_t required_size, char* buff, IOFunc io_func);

private:
    void HandleClient(int client_socket);
    void RunThread(int client_socket);

    std::size_t thread_limit_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::vector<std::thread> threads_;
    std::atomic<bool> stop_requested_;
    int server_socket_ = -1;
};

} // namespace matrix_service