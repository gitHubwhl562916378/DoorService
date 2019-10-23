#include <iostream>
#include "spdlog/spdlog.h"
#include "KoalaDoorSession.h"
#include "TcpServer.h"

Server::Server(boost::asio::io_service &io_service, int port) : io_service_(io_service),
                                                                acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
    async_accept();
}

void Server::insert_session(const std::shared_ptr<Session> session)
{
    if (!session)
        return;

    std::lock_guard<std::mutex> lock(mtx_);
    std::string ip_str = session->socket().local_endpoint().address().to_string();
    session_map_.insert(std::make_pair(ip_str, session));
    spdlog::info("new session: {}, session size: {}", ip_str, session_map_.size());
}

void Server::remove_session(const std::string ip)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto iter = session_map_.find(ip);
    if (iter == session_map_.end())
        return;
    session_map_.erase(iter);
    spdlog::info("session size: {}", session_map_.size());
    std::cout << "count : " << iter->second.use_count() << " " << __LINE__ << std::endl;
}

void Server::open_door(const std::string ip)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto iter = session_map_.find(ip);
    if (iter == session_map_.end())
        return;
    iter->second->open_door();
}

void Server::close_door(const std::string ip)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto iter = session_map_.find(ip);
    if (iter == session_map_.end())
        return;
    iter->second->close_door();
}

void Server::async_accept()
{
    std::shared_ptr<Session> new_session(new KoalaDoorSession(io_service_, this));
    acceptor_.async_accept(new_session->socket(), [=](const boost::system::error_code &ec) {
        if (!ec)
        {
            insert_session(new_session);
            new_session->async_read();
        }
        async_accept();
    });
}