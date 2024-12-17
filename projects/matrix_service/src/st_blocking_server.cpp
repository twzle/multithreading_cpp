#include "st_blocking_server.hpp"
#include "utility.hpp"

#include "executor/executor.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cassert>
#include <iostream>
#include <utility>

namespace matrix_service {

StBlockingServer::StBlockingServer(Config conf)
    : Server(std::move(conf))
{
    VALIDATE_LINUX_CALL(server_socket_ = socket(AF_INET, SOCK_STREAM, 0));

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    VALIDATE_LINUX_CALL(inet_pton(AF_INET, Cfg().listening_address.c_str(), &server_address.sin_addr));
    server_address.sin_port = htons(Cfg().port);

    VALIDATE_LINUX_CALL(bind(server_socket_, (struct sockaddr*) &server_address, sizeof(server_address)));

    constexpr std::uint32_t queue_size = 5;
    listen(server_socket_, queue_size);
}

StBlockingServer::~StBlockingServer()
{
    if (server_socket_ != -1)
    {
        // Может здесь и будет ошибка, но зато мы точно закроем, если не был закрыт
        shutdown(server_socket_, SHUT_RDWR);
        close(server_socket_);
    }

    if (client_socket_ != -1)
    {
        shutdown(client_socket_, SHUT_RDWR);
        close(client_socket_);
    }
}

void StBlockingServer::OnStop()
{
    if (server_socket_ != -1)
        shutdown(server_socket_, SHUT_RDWR);
    if (client_socket_ != -1)
        shutdown(client_socket_, SHUT_RDWR);
}

template<typename IOFunc>
bool StBlockingServer::TryIOEnough(std::size_t required_size, char* buff, IOFunc io_func)
{
    std::size_t processed_cnt = 0;
    while (processed_cnt < required_size)
    {
        int read_res = io_func(client_socket_, buff + processed_cnt, required_size - processed_cnt);
        if (read_res > 0)
            processed_cnt += read_res;
        else if (read_res == 0) // Shutdown, не важно - на чтение или на запись - продолжать нет смысла
        {
            client_send_shutdown_ = true;
            break;
        }
        else
        {
            if (!StopRequired()) // Корректное завершение по сигналу
                RaiseLinuxCallError(__LINE__, __FILE__, "read/write()", "in StBlockingServer::TryIOEnough");
            else
                assert(errno == EINTR);
            break;
        }
    }

    return processed_cnt == required_size && !StopRequired();
}

void StBlockingServer::Run()
{
    while (!StopRequired())
    {
        // 1. Прием соединения
        client_socket_ = accept(server_socket_, nullptr, nullptr);
        if (client_socket_ == -1)
        {
            if (!StopRequired())
                RaiseLinuxCallError(__LINE__, __FILE__, "accept", "in StBlockingServer::Run");
            else {
                assert(errno == EINVAL || errno == EINTR); // INVAL - из-за shutdown()
                break;
            }
        }

        bool need_read_next = true;
        while (need_read_next)
        {
            // 2. Чтение запроса
            // - Реализует простой протокол: {размер content (4 байта)} + {content}
            // - "Битые" запросы исполнению не подлежат (просто закрываем соедиение)
            // - При Stop() последний запрос не исполняется, в случае записи данных - запись обрывается
            // - При посылке ошибочного ProcedureData - закрываем соединение
            // TODO: Не копируйте это место из класса в класс - постарайтесь обобщить!
            int content_size = 0;
            if (!TryIOEnough(sizeof(content_size), (char*) &content_size, &read))
                break;
            if (content_size == 0)
                continue;

            // Если был shutdown() со стороны клиента, то просто еще раз получим 0
            std::string request(content_size, '\0');
            if (!TryIOEnough(content_size, &request[0], &read))
                break;


            // 3. Исполнение
            auto result = ExecuteProcedure(request);


            // 4. Запись ответа
            content_size = result.first.size();
            if (!TryIOEnough(sizeof(content_size), (char*) &content_size, &write) ||
                !TryIOEnough(content_size, result.first.data(), &write))
            {
                break;
            }

            // 5. Нужно ли читать следующий запрос?
            if (!result.second || client_send_shutdown_)
                need_read_next = false;
            else
                need_read_next = Cfg().keepalive;
        }

        // Shutown() ранее, в OnStop()
        if (!StopRequired())
            VALIDATE_LINUX_CALL(shutdown(client_socket_, SHUT_RDWR));

        close(client_socket_);
        client_socket_ = -1;
        client_send_shutdown_ = false;
    }
}

void StBlockingServer::Swap(StBlockingServer& another)
{
    std::swap(server_socket_, another.server_socket_);

    std::swap(client_socket_, another.client_socket_);
    std::swap(client_send_shutdown_, another.client_send_shutdown_);
}

} // namespace matrix_service
