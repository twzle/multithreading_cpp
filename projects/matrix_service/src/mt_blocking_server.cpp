#include "mt_blocking_server.hpp"
#include "utility.hpp"

#include "executor/executor.hpp"
#include <iostream>
#include <algorithm>
#include <cassert>

namespace matrix_service
{
    MtBlockingServer::MtBlockingServer(Config conf)
        : Server(std::move(conf))
    {
        VALIDATE_LINUX_CALL(server_socket_ = socket(AF_INET, SOCK_STREAM, 0));

        int reuse = 1;
        VALIDATE_LINUX_CALL(setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)));

        sockaddr_in server_address{};
        server_address.sin_family = AF_INET;
        VALIDATE_LINUX_CALL(inet_pton(AF_INET, Cfg().listening_address.c_str(), &server_address.sin_addr));
        server_address.sin_port = htons(Cfg().port);

        VALIDATE_LINUX_CALL(bind(server_socket_, (struct sockaddr *)&server_address, sizeof(server_address)));

        constexpr std::uint32_t queue_size = 5;
        listen(server_socket_, queue_size);
    }

    MtBlockingServer::~MtBlockingServer()
    {
        OnStop();
        for (auto &thread : threads_)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

    void MtBlockingServer::OnStop()
    {
        stop_requested_ = true;
        if (server_socket_ != -1)
        {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
        }

        std::unique_lock<std::mutex> lock(mutex_);
        cv_.notify_all();
    }

    void MtBlockingServer::HandleClient(int client_socket)
    {
        try
        {
            while (!stop_requested_)
            {
                int content_size = 0;
                if (!TryIOEnough(client_socket, sizeof(content_size), (char *)&content_size, &read))
                {
                    break;
                }

                if (content_size == 0)
                {
                    continue;
                }

                std::string request(content_size, '\0');
                if (!TryIOEnough(client_socket, content_size, &request[0], &read))
                {
                    break;
                }

                auto result = ExecuteProcedure(request);

                content_size = result.first.size();
                if (!TryIOEnough(client_socket, sizeof(content_size), (char *)&content_size, &write) ||
                    !TryIOEnough(client_socket, content_size, result.first.data(), &write))
                {
                    break;
                }

                // Если пакет битый или нет keepalive, то не нужно читать дальше
                if (!result.second || !Cfg().keepalive)
                {
                    break;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in HandleClient: " << e.what() << "\n";
        }

        VALIDATE_LINUX_CALL(shutdown(client_socket, SHUT_RDWR));
        close(client_socket);
    }

    void MtBlockingServer::Run()
    {
        while (!stop_requested_)
        {
            int client_socket = accept(server_socket_, nullptr, nullptr);
            if (client_socket == -1)
            {
                if (!stop_requested_)
                {
                    RaiseLinuxCallError(__LINE__, __FILE__, "accept", "in MtBlockingServer::Run");
                }
                break;
            }

            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]
                         { return threads_.size() < Cfg().thread_limit || stop_requested_; });

                if (stop_requested_)
                {
                    close(client_socket);
                    break;
                }

                threads_.emplace_back([this, client_socket]
                                      { RunThread(client_socket); });
            }
        }
    }

    void MtBlockingServer::RunThread(int client_socket)
    {
        HandleClient(client_socket);

        {
            std::unique_lock<std::mutex> lock(mutex_);
            threads_.erase(std::remove_if(threads_.begin(), threads_.end(), [](const std::thread &t)
                                          { return !t.joinable(); }),
                           threads_.end());
            cv_.notify_all();
        }
    }

    template <typename IOFunc>
    bool MtBlockingServer::TryIOEnough(int client_socket, std::size_t required_size, char *buff, IOFunc io_func)
    {
        std::size_t processed_cnt = 0;
        while (processed_cnt < required_size && !stop_requested_)
        {
            int result = io_func(client_socket, buff + processed_cnt, required_size - processed_cnt);
            if (result > 0)
            {
                processed_cnt += result;
            }
            else if (result == 0)
            {
                break;
            }
            else
            {
                if (!stop_requested_)
                {
                    RaiseLinuxCallError(__LINE__, __FILE__, "read/write", "in MtBlockingServer::TryIOEnough");
                }
                break;
            }
        }
        return processed_cnt == required_size && !stop_requested_;
    }

} // namespace matrix_service
