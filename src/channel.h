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

    typedef std::function<void(Command::Response&)> callback_func_t;

    void sendCmd(Command::Request&& req, callback_func_t callback);

private:
    void run();
    void parseIncomeData(const std::string& data);

    virtual void onConnected() = 0;

private:
    net::io_context& ioc_;
    std::string host_;
    std::string port_;
    std::string path_;
    std::string socks_proxy_;

    std::unique_ptr<std::thread> thread_;

    struct Cmd {
        Command::Request req;
        callback_func_t cb;
    };

    ConcurrentQueueT<Cmd> outq_;
    ConcurrentQueueT<std::string> inq_;
    std::deque<Cmd> waiting_resp_q_;
};
