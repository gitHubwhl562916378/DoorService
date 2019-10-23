#include <iostream>
#include "spdlog/spdlog.h"
#include "spdlog/fmt/bin_to_hex.h"
#include "KoalaDoorSession.h"
#include "common.h"
#include "TcpServer.h"

KoalaDoorSession::KoalaDoorSession(boost::asio::io_service &io_service, Server *ser) : Session(io_service), hurt_timer_(io_service), server_(ser)
{
}

KoalaDoorSession::~KoalaDoorSession()
{
    hurt_timer_.cancel();
}

void KoalaDoorSession::async_read()
{
    try
    {
        auto self(shared_from_this());
        socket().async_read_some(boost::asio::buffer(buffer_.data() + rc_bytes_, buffer_.size() - rc_bytes_), [this, self](boost::system::error_code ec, size_t bytes_transfered) {
            if (!ec)
            {
                rc_bytes_ += bytes_transfered;
                spdlog::debug("received {} bytes, message: {}", bytes_transfered, spdlog::to_hex(buffer_));
                if (rc_bytes_ == 6)
                {
                    if (!::strncmp(g_hurt, buffer_.data(), sizeof(g_hurt)))
                    {
                        spdlog::debug("socket {} is alive", socket().local_endpoint().address().to_string());
                        ::memset(buffer_.data(), 0, buffer_.size());
                        rc_bytes_ = 0;
                        hurt_count_++;
                        if (!hurt_timer_is_start_)
                        {
                            hurt_timeout(boost::system::error_code());
                            hurt_timer_is_start_ = true;
                        }
                    }
                }
                else if (rc_bytes_ == 8)
                {
                    if (!::strncmp(g_state_re_open, buffer_.data(), sizeof(g_state_re_open)))
                    {
                        spdlog::debug("door state is open");
                    }
                    else if (!::strncmp(g_state_re_close, buffer_.data(), sizeof(g_state_re_close)))
                    {
                        spdlog::debug("door state is close");
                    }
                    else
                    {
                        spdlog::debug("unknow message: {}", buffer_.data());
                    }
                    ::memset(buffer_.data(), 0, buffer_.size());
                    rc_bytes_ = 0;
                }
                async_read();
            }
            else
            {
                spdlog::info("session closed: {}", socket().local_endpoint().address().to_string());
                server_->remove_session(socket().local_endpoint().address().to_string());
            }
        });
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Session::async_read exception: {}", e.what());
    }
}

void KoalaDoorSession::search_state()
{
    async_write(g_state, sizeof(g_state));
}

void KoalaDoorSession::close_door()
{
    async_write(g_close, sizeof(g_open));
}

void KoalaDoorSession::open_door()
{
    async_write(g_open, sizeof(g_open));
}

void KoalaDoorSession::async_write(const char *buffer, const int length)
{
    try
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket(), boost::asio::buffer(buffer, length), [=](boost::system::error_code ec, size_t bytes_writed) {
            if (!ec)
            {
                spdlog::debug("{} bytes writed: {}", bytes_writed, spdlog::to_hex(std::vector<char>(buffer, buffer + length)));
            }
            else
            {
                spdlog::info("session closed: {}", socket().local_endpoint().address().to_string());
                server_->remove_session(socket().local_endpoint().address().to_string());
            }
        });
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Session::async_write exception: {}", e.what());
    }
}

void KoalaDoorSession::hurt_timeout(boost::system::error_code ec)
{
    try
    {
        if (!ec)
        {
            if (hurt_count_ == 0)
            {
                spdlog::info("session hurt lost: {}", socket().local_endpoint().address().to_string());
                socket().cancel();
                server_->remove_session(socket().local_endpoint().address().to_string());
                hurt_timer_.cancel();
                return;
            }
            hurt_count_ = 0;
            hurt_timer_.expires_from_now(boost::posix_time::milliseconds(5000));
            hurt_timer_.async_wait(std::bind(&KoalaDoorSession::hurt_timeout, this, std::placeholders::_1));
        }
    }
    catch (const std::exception &e)
    {
        spdlog::warn("Session::hurt_timeout exception: {}", e.what());
    }
}