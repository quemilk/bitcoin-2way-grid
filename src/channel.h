#pragma once

#include "ws_session.h"
#include <thread>


class Channel {
public:
    Channel(net::io_context& ioc, 
        const std::string& host, const std::string& port, const std::string& path,
        const std::string socks_proxy = "");

    virtual ~Channel();

private:
    void run();
    void parseIncomeData(const std::string& data);

    virtual void onConnected() = 0;

protected:
    std::shared_ptr<WSSession> ws_session_;
    std::unique_ptr<std::thread> thread_;
};
