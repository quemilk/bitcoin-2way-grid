#pragma once

#include "ws_session.h"
#include <thread>


class Channel {
public:
    Channel(const std::string& host, const std::string& port, const std::string& path, const std::string socks_proxy = "");
    virtual ~Channel();


private:
    void poll_run();

private:
    net::io_context ioc_;
    std::shared_ptr<WSSession> ws_session_;
    std::unique_ptr<std::thread> thread_;
};
