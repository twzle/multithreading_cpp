#include "st_nonblocking_server.hpp"
#include "utility.hpp"


namespace matrix_service
{

    StNonblockingServer::StNonblockingServer(Config conf)
        : Server(std::move(conf))
    {
        VALIDATE_LINUX_CALL(server_socket_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0));

        int reuse = 1;
        VALIDATE_LINUX_CALL(setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)));

        sockaddr_in server_address = {};
        server_address.sin_family = AF_INET;
        VALIDATE_LINUX_CALL(inet_pton(AF_INET, conf.listening_address.c_str(), &server_address.sin_addr));
        server_address.sin_port = htons(conf.port);

        VALIDATE_LINUX_CALL(bind(server_socket_, (struct sockaddr *)&server_address, sizeof(server_address)));
        VALIDATE_LINUX_CALL(listen(server_socket_, 5));

        SetupEpoll();
    }

    StNonblockingServer::~StNonblockingServer()
    {
        if (server_socket_ != -1)
        {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
        }
        if (epoll_fd_ != -1)
            close(epoll_fd_);
    }

    void StNonblockingServer::SetupEpoll()
    {
        epoll_fd_ = epoll_create1(0);
        if (epoll_fd_ == -1)
            RaiseLinuxCallError(__LINE__, __FILE__, "epoll_create1", "failed to create epoll instance");

        epoll_event event = {};
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = server_socket_;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_socket_, &event) == -1)
            RaiseLinuxCallError(__LINE__, __FILE__, "epoll_ctl", "failed to add server socket to epoll");
    }

    void StNonblockingServer::Run()
    {
        ProcessEvents();
    }

    void StNonblockingServer::OnStop()
    {
        for (auto &client : clients_)
        {
            if (client.first != -1)
            {
                shutdown(client.first, SHUT_RDWR);
                close(client.first);
            }
        }

        if (epoll_fd_ != -1)
        {
            close(epoll_fd_);
        }

        if (server_socket_ != -1)
        {
            shutdown(server_socket_, SHUT_RDWR);
            close(server_socket_);
        }
    }

    void StNonblockingServer::ProcessEvents()
    {
        constexpr int max_events = 10;
        epoll_event events[max_events];

        while (!StopRequired())
        {
            int event_count = epoll_wait(epoll_fd_, events, max_events, -1);
            if (event_count == -1)
            {
                if (errno == EINTR)
                    continue;
                RaiseLinuxCallError(__LINE__, __FILE__, "epoll_wait", "failed to wait for events");
            }

            for (int i = 0; i < event_count; ++i)
            {
                int client_socket = events[i].data.fd;

                if (client_socket == server_socket_)
                {
                    int new_client = accept(server_socket_, nullptr, nullptr);
                    if (new_client == -1)
                        continue;

                    int flags = fcntl(new_client, F_GETFL, 0);
                    fcntl(new_client, F_SETFL, flags | O_NONBLOCK);

                    epoll_event event = {};
                    event.events = EPOLLIN;
                    event.data.fd = new_client;
                    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, new_client, &event) == -1)
                    {
                        close(new_client);
                        continue;
                    }

                    clients_[new_client] = {};
                }
                else
                {
                    if (events[i].events & EPOLLIN)
                    {
                        HandleClientRead(client_socket);
                    }
                    if (events[i].events & EPOLLOUT)
                    {
                        HandleClientWrite(client_socket);
                    }
                }
            }
        }
    }

    void StNonblockingServer::HandleClientRead(int client_socket)
    {
        auto &state = clients_[client_socket];

        if (state.read_buffer.empty())
            state.read_buffer.resize(4);

        auto io_func = [](int sock, char *buffer, std::size_t size) -> int
        {
            return read(sock, buffer, size);
        };

        if (!TryIOEnough(client_socket, state.read_buffer.size(), state.read_buffer.data(), io_func, state.read_offset))
        {
            CloseClient(client_socket);
            return;
        }

        if (state.read_offset < 4)
            return;


        if (state.read_buffer.size() == 4)
        {
            int content_size = *reinterpret_cast<int *>(state.read_buffer.data());
            state.read_buffer.resize(4 + content_size);
        }

        if (!TryIOEnough(client_socket, state.read_buffer.size(), state.read_buffer.data(), io_func, state.read_offset))
        {
            CloseClient(client_socket);
            return;
        }

        if (state.read_offset == state.read_buffer.size())
        {
            std::string request(state.read_buffer.begin() + 4, state.read_buffer.end());
            auto response = ExecuteProcedure(request);
            state.is_closing = !response.second;

            int response_size = response.first.size();
            state.write_buffer.resize(sizeof(response_size) + response_size);
            std::memcpy(state.write_buffer.data(), &response_size, sizeof(response_size));
            std::memcpy(
                state.write_buffer.data() + sizeof(response_size),
                response.first.data(),
                response_size);

            state.read_buffer.clear();
            state.read_offset = 0;

            // Обновляем epoll на запись
            epoll_event event = {};
            event.events = EPOLLOUT;
            event.data.fd = client_socket;
            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_socket, &event);
        }
    }

    void StNonblockingServer::HandleClientWrite(int client_socket)
    {
        auto &state = clients_[client_socket];

        auto io_func = [](int sock, char *buffer, std::size_t size) -> int
        {
            return write(sock, buffer, size);
        };


        if (!TryIOEnough(
            client_socket, 
            state.write_buffer.size(), 
            state.write_buffer.data(), 
            io_func, 
            state.write_offset))
        {
            CloseClient(client_socket);
            return;
        }

        if (state.write_offset == state.write_buffer.size())
        {
            state.write_buffer.clear();
            state.write_offset = 0;

            // Если пакет не был битым и keepalive == true, то нужно читать следующий запрос
            bool read_next = Cfg().keepalive && !state.is_closing;
            if (read_next)
            {
                // Обновляем epoll на чтение
                epoll_event event = {};
                event.events = EPOLLIN;
                event.data.fd = client_socket;
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_socket, &event);
            }
            else
            {
                CloseClient(client_socket);
            }
        }
    }

    void StNonblockingServer::CloseClient(int client_socket)
    {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_socket, nullptr);
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        clients_.erase(client_socket);
    }

    bool StNonblockingServer::TryIOEnough(
        int client_socket,
        std::size_t required_size,
        char *buff,
        std::function<int(int, char *, std::size_t)> io_func,
        std::size_t &processed_cnt)
    {
        while (processed_cnt < required_size)
        {
            int res = io_func(client_socket, buff + processed_cnt, required_size - processed_cnt);
            if (res > 0)
            {
                processed_cnt += res;
            }
            else if (res == 0)
            {
                return false;
            }
            else
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }
        }

        return true;
    }

    void StNonblockingServer::Swap(StNonblockingServer &another)
    {
        std::swap(server_socket_, another.server_socket_);
        std::swap(epoll_fd_, another.epoll_fd_);
        std::swap(clients_, another.clients_);
    }

} // namespace matrix_service
