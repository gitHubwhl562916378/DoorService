#pragma once
#include <boost/threadpool.hpp>
#include "cpprest/http_listener.h"
#include "TcpServer.h"

using namespace web;
using namespace http;
using namespace http::experimental::listener;
class HttpServer
{
public:
    explicit HttpServer(Server *door_server,const int port);
    ~HttpServer();
    std::string EndPoint();
    pplx::task<void> Accept();
    pplx::task<void> Shutdown();

private:
    void OnRequest(http_request);
    void HandOpenDoor(http_request);
    void HandCloseDoor(http_request);

    Server *door_server_{nullptr};
    std::mutex hander_mtx_;
    boost::threadpool::pool thr_poor_;
    http_listener listener_;
    std::map<std::string,std::function<void(http_request)>> handler_map_;
};