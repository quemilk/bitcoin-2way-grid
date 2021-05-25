#include "channel.h"
#include "logger.h"


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

                for (;;) {
                    Cmd cmd;
                    while (outq_.pop(&cmd, std::chrono::milliseconds(100))) {
                        ws_session_->send(cmd.req.data);
                        waiting_resp_q_.emplace_back(std::move(cmd));
                    }

                    while (ws_session_->canRead()) {
                        std::string data;
                        ws_session_->read(&data);

                        parseIncomeData(data);
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
    Cmd cmd;
    cmd.req = std::move(req);
    cmd.cb = callback;
    outq_.push(std::move(cmd));
}
