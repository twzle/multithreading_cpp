#pragma once

#include "server.hpp"
#include "utility.hpp"

#include "executor/executor.hpp"

#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <iostream>
#include <utility>
#include <coroutine>
#include <optional>
#include <functional>
#include <unordered_map>
#include <vector>

namespace matrix_service
{
    struct ClientState
    {
        std::vector<char> read_buffer;
        std::size_t read_offset = 0;

        std::vector<char> write_buffer;
        std::size_t write_offset = 0;

        bool is_closing = false;
    };

    class StNonblockingServer : public Server
    {
    public:
        explicit StNonblockingServer(Config conf);
        StNonblockingServer(const StNonblockingServer &) = delete;
        StNonblockingServer &operator=(const StNonblockingServer &) = delete;

        StNonblockingServer(StNonblockingServer &&another)
            : Server(std::move(another))
        {
            Swap(another);
        }
        StNonblockingServer &operator=(StNonblockingServer &&another)
        {
            static_cast<Server &>(*this) = std::move(another);
            Swap(another);
            return *this;
        }

        ~StNonblockingServer();

        void Run() override;

    private:
        int server_socket_ = -1;
        int epoll_fd_ = -1;
        std::unordered_map<int, ClientState> clients_;

        void SetupEpoll();
        void ProcessEvents();
        void HandleClientRead(int client_socket);
        void HandleClientWrite(int client_socket);
        void CloseClient(int client_socket);
        void OnStop() override;

        bool TryIOEnough(
            int client_socket,
            std::size_t required_size,
            char *buff,
            std::function<int(int, char *, std::size_t)> io_func,
            std::size_t &processed_cnt);

        void Swap(StNonblockingServer &another);
    };
}
