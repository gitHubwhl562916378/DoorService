#include <thread>
#include <chrono>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "Poco/Util/XMLConfiguration.h"
#include "HttpServer.h"
#include "TcpServer.h"

std::mutex mtx;
std::condition_variable cv_;
void handleUserInterrupt(int signal)
{
    if (signal == SIGINT)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "CRTL + C pressed, now exiting" << std::endl;
        cv_.notify_one();
    }
}

int main(int argc, char *argv[])
{
    try
    {
        std::string config_file = "config.xml";
        Poco::AutoPtr<Poco::Util::XMLConfiguration> configuration = new Poco::Util::XMLConfiguration;
        configuration->load(config_file);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/daily.txt", 2, 30);
        file_sink->set_level(spdlog::level::level_enum(configuration->getInt("Logs.level[@file]")));
        spdlog::set_default_logger(std::shared_ptr<spdlog::logger>(new spdlog::logger("multi_sink", {console_sink, file_sink})));
        spdlog::set_level(spdlog::level::level_enum(configuration->getInt("Logs.level[@console]")));

        Server *tcp_server_ptr{nullptr};
        boost::asio::io_service *tcp_io_service{nullptr};
        std::thread tcp_thread([&] {
            tcp_io_service = new boost::asio::io_service;
            boost::asio::io_service::work work(*tcp_io_service);
            tcp_server_ptr = new Server(*tcp_io_service, 52104);
            tcp_io_service->run();
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));
        HttpServer server(tcp_server_ptr, configuration->getInt("HttpServer.host[@port]"));
        server.Accept().wait();
        spdlog::info("Microsvc is listening request on {}", server.EndPoint());
        spdlog::warn("Press 'CTRL + C' key to quit...");

        signal(SIGINT, handleUserInterrupt);
        std::unique_lock<std::mutex> lock(mtx);
        cv_.wait(lock);
        server.Shutdown().wait();

        tcp_io_service->stop();
        if (tcp_thread.joinable())
        {
            tcp_thread.join();
        }
    }
    catch (std::exception &e)
    {
        spdlog::critical("main exception: {}", e.what());
    }
    return 0;
}