#include "channel.h"
#include "logger.h"
#include <chrono>


Channel::Channel(net::io_context& ioc,
    const std::string& host, const std::string& port, const std::string& path,
    const std::string socks_proxy) :
ioc_(ioc),
host_(host),
port_(port),
path_(path),
socks_proxy_(socks_proxy) {
    thread_.reset(new std::thread(std::bind(&Channel::run, this)));
}

Channel::~Channel() {
}

void Channel::run() {
    for (;;) {
        try {
            ssl::context ctx{ ssl::context::tlsv12_client };
            auto ws_session_ = std::make_shared<WSSession>(ioc_, ctx, host_, port_, path_);
            if (!socks_proxy_.empty())
                ws_session_->setSocksProxy(socks_proxy_.c_str());

            ws_session_->start();
            if (ws_session_->waitUtilConnected(std::chrono::seconds(10))) {
                this->onConnected();

                auto last_read = std::chrono::steady_clock::now();

                inq_.clear();
                for (auto& v : waiting_resp_q_) {
                    ws_session_->send(v.req.data);
                }

                for (;;) {
                    ws_session_->async_read(
                        [&](std::string& data) {
                            inq_.push(std::move(data));
                        }
                    );

                    Cmd cmd;
                    while (outq_.pop(&cmd, std::chrono::milliseconds(10))) {
                        waiting_resp_q_.push_back(cmd);
                        ws_session_->send(cmd.req.data);
                    }

                    std::string indata;
                    while (inq_.tryPop(&indata)) {
                        last_read = std::chrono::steady_clock::now();
                        if (indata.empty())
                            throw std::runtime_error("connection closed!");

                        parseIncomeData(indata);
                    }

                    auto now = std::chrono::steady_clock::now();
                    if (now - last_read > std::chrono::seconds(15)) {
                        last_read = now;
                        ws_session_->ping();
                        // TODO check pong
                    }
                }
            } else {
                LOG(error) << "connect failed!";
            }
        } catch (const std::exception& e) {
            LOG(error) << "exception: " << e.what();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Channel::parseIncomeData(const std::string& data) {
    Command::Response resp;
    if (Command::parseReceivedData(data, &resp)) {
        for (auto itr = waiting_resp_q_.begin(); itr != waiting_resp_q_.end();) {
            auto& req = itr->req;
            if (resp.id == req.id && (resp.op == req.op || resp.op == "error")) {
                if (itr->cb)
                    itr->cb(resp);
                itr = waiting_resp_q_.erase(itr);
                break;
            }
            ++itr;
        }
    }
}

void Channel::sendCmd(Command::Request&& req, callback_func_t callback) {
    if (req.op != "login")
        LOG(debug) << "sendCmd. id=" << req.id << ", op=" << req.op << ", data=" << req.data;
    Cmd cmd;
    cmd.req = std::move(req);
    cmd.cb = callback;
    outq_.push(std::move(cmd));
}
