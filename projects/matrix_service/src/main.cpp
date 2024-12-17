#include "server.hpp"
#include "st_blocking_server.hpp"

#include "cxxopts.hpp"

#include <signal.h>

#include <cassert>
#include <csignal>
#include <memory>
#include <iostream>


// Чтобы ловить сигналы, нужен глобальный обработчик
std::unique_ptr<matrix_service::Server> g_server;

void StopHandler(int)
{
    if (g_server)
        g_server->Stop();
}


int main(int argc, char* argv[])
{
    using namespace std::string_literals;

    static constexpr int ArgErrorExitCode = 1;
    constexpr std::string_view AllowedServerType = "st_blocking";

    matrix_service::Server::Config conf;

    std::string server_type;
    cxxopts::Options opts(argv[0], "- options for matrix server");
    opts.add_options()
        ("h,help", "show help")
        ("s,server_type", "type of the server, allowed: "s + AllowedServerType.data(), cxxopts::value<std::string>(server_type))
        ("a,address", "the listening address", cxxopts::value<std::string>(conf.listening_address)->default_value("0.0.0.0"s))
        ("p,port", "the port for app", cxxopts::value<std::uint16_t>(conf.port)->default_value("8080"s))
        ("k,keepalive", "should server support keepalive mode", cxxopts::value<bool>(conf.keepalive)->default_value("false"s));

    try
    {
        cxxopts::ParseResult parsed_opts = opts.parse(argc, argv);
        if (parsed_opts.count("help"))
        {
            std::cout << opts.help() << std::endl;
            return 0;
        }
    }
    catch (const cxxopts::exceptions::exception& e)
    {
        std::cerr << "Error parsing option: " << e.what() << std::endl;
        std::cerr << "Usage: " << opts.help() << std::endl;
        return ArgErrorExitCode;
    }

    if (server_type == "st_blocking")
        g_server = std::make_unique<matrix_service::StBlockingServer>(std::move(conf));
    else
    {
        std::cerr << "Unknown type of server: '" << server_type << "', allowed: " << AllowedServerType << std::endl;
        return ArgErrorExitCode;
    }

    std::signal(SIGINT, StopHandler);
    assert(g_server);
    g_server->Run();

    return 0;
}
