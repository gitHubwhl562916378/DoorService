#pragma once
#include "session.h"
#include <boost/asio/deadline_timer.hpp>

class Server;
class KoalaDoorSession : public Session
{
public:
    KoalaDoorSession(boost::asio::io_service &io_service, Server *ser);
    ~KoalaDoorSession();

    void async_read() override;

    void search_state();

    void close_door() override;
    
    void open_door() override;

private:
    void async_write(const char *buffer, const int length);
    void hurt_timeout(boost::system::error_code ec);

    boost::asio::deadline_timer hurt_timer_;
    std::array<char, 8> buffer_;
    std::atomic_int rc_bytes_, hurt_count_;
    bool hurt_timer_is_start_ = false;

    Server *server_{nullptr};
};