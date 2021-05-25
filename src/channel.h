#pragma once

#include "ws_session.h"
#include "command.h"
#include "concurrent_queue.h"
#include <thread>
#include <functional>
#include <deque>

class Channel {
public:
    Channel(net::io_context& ioc, 
        const std::string& host, const std::string& port, const std::string& path,
        const std::string socks_proxy = "");

    virtual ~Channel();

    void sendCmd(Command::Request&& req, std::function<void(const std::string&)> callback);

private:
    void run();
    void parseIncomeData(const std::string& data);

    virtual void onConnected() = 0;

private:
    std::shared_ptr<WSSession> ws_session_;
    std::unique_ptr<std::thread> thread_;

    struct Cmd {
        Command::Request req;
        std::function<void(const std::string&)> cb;
    };

    ConcurrentQueueT<Cmd> outq_;
    std::deque<Cmd> waiting_resp_q_;
};
