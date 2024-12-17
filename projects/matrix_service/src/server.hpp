#pragma once

#include <cstdint>
#include <string>

namespace matrix_service {

class Server
{
public:
    struct Config
    {
        std::string listening_address;
        std::uint16_t port = 0;

        // Держать ли соединение с клиентами, ожидая новых запросов, или закрыть сразу после отправки ответа?
        bool keepalive = false;
    };

public:
    explicit Server(Config conf)
        : cfg_(std::move(conf))
    {}
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&& another)
    {
        Swap(another);
    }
    Server& operator=(Server&& another)
    {
        Swap(another);
        return *this;
    }

    virtual ~Server() {}

    virtual void Run() = 0; // Должна делать Join(), если многопоточна
    void Stop()
    {
        stop_required_ = true;
        OnStop();
    }

    const Config& Cfg() const { return cfg_; }

protected:
    void Swap(Server& another)
    {
        std::swap(stop_required_, another.stop_required_);
        std::swap(cfg_, another.cfg_);
    }
    bool StopRequired() const { return stop_required_; }

    virtual void OnStop() {}

private:
    Config cfg_;
    bool stop_required_ = false;
};

} // namespace matrix_service
