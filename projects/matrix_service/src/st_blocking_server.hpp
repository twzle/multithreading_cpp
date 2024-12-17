#pragma once

#include "server.hpp"

namespace matrix_service {

class StBlockingServer : public Server
{
public:
    explicit StBlockingServer(Config conf);
    StBlockingServer(const StBlockingServer&) = delete;
    StBlockingServer& operator=(const StBlockingServer&) = delete;

    StBlockingServer(StBlockingServer&& another)
        : Server(std::move(another))
    {
        Swap(another);
    }
    StBlockingServer& operator=(StBlockingServer&& another)
    {
        static_cast<Server&>(*this) = std::move(another);
        Swap(another);
        return *this;
    }

    ~StBlockingServer();

    void Run() override;

private:
    void Swap(StBlockingServer& another);

    template<typename IOFunc>
    bool TryIOEnough(std::size_t required_size, char* buff, IOFunc io_fund);

    void OnStop() override;

private:
    int server_socket_ = -1;

    int client_socket_ = -1;
    bool client_send_shutdown_ = false;
};

} // namespace matrix_service
