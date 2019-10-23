#include <iostream>
#include "spdlog/spdlog.h"
#include "session.h"
#include "common.h"
#include "TcpServer.h"

Session::Session(boost::asio::io_service &io_service) : socket_(io_service)
{
}

Session::~Session()
{
    spdlog::info("session destructor: {}", socket_.local_endpoint().address().to_string());
}

boost::asio::ip::tcp::socket &Session::socket()
{
    return socket_;
}