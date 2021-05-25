#include "channel.h"
#include "logger.h"
#include <thread>


Channel::Channel(net::io_context& ioc,
    const std::string& host, const std::string& port, const std::string& path,
    const std::string socks_proxy) {
    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    ws_session_ = std::make_shared<WSSession>(ioc, ctx, host, port, path);
    if (!socks_proxy.empty())
        ws_session_->setSocksProxy(socks_proxy.c_str());

    thread_.reset(new std::thread(std::bind(&Channel::run, this)));
}

Channel::~Channel() {
}

void Channel::run() {
    for (;;) {
        try {
            ws_session_->start();
            if (ws_session_->waitUtilConnected(std::chrono::seconds(10))) {
                this->onConnected();

                while (ws_session_->canRead()) {
                    std::string data;
                    ws_session_->read(&data);

                    parseIncomeData(data);
                }
                Sleep(100000);
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

}
