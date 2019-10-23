#include "spdlog/spdlog.h"
#include "boost/thread.hpp"
#include "HttpServer.h"

HttpServer::HttpServer(Server *door_server, const int port) : door_server_(door_server)
{
    listener_ = http_listener("http://0.0.0.0:" + std::to_string(port));
    listener_.support(std::bind(&HttpServer::OnRequest, this, std::placeholders::_1));

    thr_poor_.size_controller().resize(std::thread::hardware_concurrency() * 2);
    handler_map_.insert(std::map<std::string, std::function<void(http_request)>>::value_type("/api/v1/open-door", std::bind(&HttpServer::HandOpenDoor, this, std::placeholders::_1)));
    handler_map_.insert(std::map<std::string, std::function<void(http_request)>>::value_type("/api/v1/close-door", std::bind(&HttpServer::HandCloseDoor, this, std::placeholders::_1)));
}

HttpServer::~HttpServer()
{
}

std::string HttpServer::EndPoint()
{
    return listener_.uri().to_string();
}

pplx::task<void> HttpServer::Accept()
{
    return listener_.open();
}

pplx::task<void> HttpServer::Shutdown()
{
    return listener_.close();
}

void HttpServer::OnRequest(http_request message)
{
    std::string url_path = message.relative_uri().path();
    spdlog::info("Received request, path: {}", url_path);

    if (message.method() == methods::POST)
    {
        thr_poor_.schedule([=] {
            auto response = json::value::object();
            try
            {
                std::function<void(http_request)> handler;
                bool is_valid_path = false;
                {
                    std::lock_guard<std::mutex> lock(hander_mtx_);
                    auto iter = handler_map_.find(url_path);
                    if (iter != handler_map_.end())
                    {
                        handler = iter->second;
                        is_valid_path = true;
                    }
                }

                if (is_valid_path)
                {
                    handler(message);
                }
                else
                {
                    response["status"] = json::value::number(20001);

                    std::string msg("request an unknown uri: ");
                    msg += url_path;

                    response["message"] = json::value::string(msg);
                    message.reply(status_codes::BadRequest, response);

                    spdlog::trace("unknown path: {}", url_path);
                }
            }
            catch (std::exception &e)
            {
                response["status"] = json::value::number(20001);
                response["message"] = json::value::string(e.what());
                message.reply(status_codes::InternalError, response);

                spdlog::error("{}:{}", __FUNCTION__, e.what());
            }
            catch (...)
            {
                response["status"] = json::value::number(20001);
                response["message"] = json::value::string("unknown error");
                message.reply(status_codes::InternalError, response);

                spdlog::error("{}: unknown error", __func__);
            }
        });
    }
    else
    {
        auto response = json::value::object();
        response["status"] = json::value::number(20001);
        response["message"] = json::value::string("unsupported methord");
        message.reply(status_codes::NotImplemented, response);

        spdlog::warn("NotImplemented methord called");
    }
}

void HttpServer::HandOpenDoor(http_request message)
{
    auto response_func = [=](const std::string &msg, bool is_ok) -> pplx::task<void> {
        auto response = json::value::object();
        response["message"] = json::value::string(msg);
        status_code code;
        if (is_ok)
        {
            response["status"] = 200;
            code = status_codes::OK;
        }
        else
        {
            response["status"] = 20001;
            code = status_codes::InternalError;
        }
        return message.reply(code, response);
    };

    try
    {
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        message.extract_json().then([=](json::value jsVal) {
                                  auto js_obj = jsVal.as_object();
                                  std::string ip = js_obj.at("ip").as_string();
                                  door_server_->open_door(ip);
                                  return response_func("success", true);
                              })
            .wait();
    }
    catch (std::exception &e)
    {
        spdlog::error("{}", e.what());

        response_func(e.what(), false);
    }
}

void HttpServer::HandCloseDoor(http_request message)
{
    auto response_func = [=](const std::string &msg, bool is_ok) -> pplx::task<void> {
        auto response = json::value::object();
        response["message"] = json::value::string(msg);
        status_code code;
        if (is_ok)
        {
            response["status"] = 200;
            code = status_codes::OK;
        }
        else
        {
            response["status"] = 20001;
            code = status_codes::InternalError;
        }
        return message.reply(code, response);
    };

    try
    {
        std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
        message.extract_json().then([=](json::value jsVal) {
                                  auto js_obj = jsVal.as_object();
                                  std::string ip = js_obj.at("ip").as_string();
                                  door_server_->close_door(ip);
                                  return response_func("success", true);
                              })
            .wait();
    }
    catch (std::exception &e)
    {
        spdlog::error("{}", e.what());

        response_func(e.what(), false);
    }
}