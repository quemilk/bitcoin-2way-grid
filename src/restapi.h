#pragma once

#include "http_session.h"
#include "command.h"
#include "concurrent_queue.h"
#include <thread>
#include <functional>
#include <deque>

class RestApi {
public:
    RestApi(net::io_context& ioc,
        const std::string& host, const std::string& port,
        const std::string socks_proxy = "");

    virtual ~RestApi();

    typedef http::request<http::string_body> req_type;
    typedef http::response<http::string_body> resp_type;


    bool setLeverage(int lever);
    bool checkOrderFilled();

private:
    bool sendCmd(const string& verbstr, const std::string& path, const std::string& reqdata, resp_type* resp);

private:
    net::io_context& ioc_;
    
    std::string host_;
    std::string port_;
    std::string socks_proxy_;
};

extern std::shared_ptr<RestApi> g_restapi;