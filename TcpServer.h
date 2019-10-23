#pragma once
#include <memory>
#include <array>
#include <boost/asio.hpp>
#include <mutex>
#include "session.h"

class Server
{
public:
    Server(boost::asio::io_service &io_service, int port);
    void insert_session(const std::shared_ptr<Session> session);
    void remove_session(const std::string ip);
    void open_door(const std::string ip);
    void close_door(const std::string ip);

private:
    void async_accept();

    boost::asio::io_service &io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::map<std::string, std::shared_ptr<Session>> session_map_;
    std::mutex mtx_;
};