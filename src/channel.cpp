#include "channel.h"


Channel::Channel(net::io_context& ioc,
    const std::string& host, const std::string& port, const std::string& path,
    const std::string socks_proxy) {
    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    ws_session_ = std::make_shared<WSSession>(ioc, ctx);
    if (!socks_proxy.empty())
        ws_session_->setSocksProxy(socks_proxy.c_str());
    ws_session_->run(host.c_str(), port.c_str(), path.c_str());

}

Channel::~Channel() {
}
