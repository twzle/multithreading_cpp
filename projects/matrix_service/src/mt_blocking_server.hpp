#pragma once

#include "server.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <map>
#include <thread>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <algorithm>
#include <cassert>
#include <queue>

namespace matrix_service
{

    class MtBlockingServer : public Server
    {
    public:
        explicit MtBlockingServer(Config conf);
        ~MtBlockingServer();
        void Run() override;
        void OnStop() override;

    private:
        void HandleClient(int client_socket);
        void RunThread(int client_socket);
        void JoinCompletedThreads();
        template <typename IOFunc>
        bool TryIOEnough(int client_socket, std::size_t required_size, char *buff, IOFunc io_func);

        std::size_t thread_limit_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::map<std::thread::id, std::thread> active_threads_;
        std::queue<std::thread::id> finished_threads_;
        std::atomic<bool> stop_requested_;
        int server_socket_ = -1;
    };
}