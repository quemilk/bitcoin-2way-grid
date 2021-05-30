#pragma once

#include "http_session.h"

class RestApi {
public:
    RestApi(net::io_context& ioc,
        const std::string& host, const std::string& port,
        const std::string socks_proxy = "");

public:
    bool setLeverage(int lever);

    int getLeverage();

private:
};

extern std::shared_ptr<RestApi> g_restapi;