#pragma once
#include <memory>
#include <atomic>
#include <boost/asio.hpp>

class Server;
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(boost::asio::io_service &io_service);
    virtual ~Session();
    boost::asio::ip::tcp::socket& socket();

    virtual void async_read() = 0;

    virtual void close_door() = 0;
    
    virtual void open_door() = 0;

private:
    boost::asio::ip::tcp::socket socket_;
};