#pragma once

#include "type.h"
#include "socks/uri.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class HttpSeesion : public std::enable_shared_from_this<HttpSeesion> {
public:
    HttpSeesion(net::io_context& ioc, ssl::context& ctx, 
        const std::string& host, const std::string& port);

    void setSocksProxy(char const* socks_server) {
        socks_server_ = socks_server;
    }

    void start();

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    void on_ssl_handshake(beast::error_code ec);

    void on_socks_proxy_resolve(beast::error_code ec, tcp::resolver::results_type results);

    void on_socks_proxy_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    void on_socks_proxy_handshake(beast::error_code ec);

    void on_fail(beast::error_code ec, char const* what);

private:
    const std::string host_;
    const std::string port_;
    std::string socks_server_;
    socks::uri socks_url_;
    int socks_version_;

    tcp::resolver resolver_;
    beast::ssl_stream<beast::tcp_stream> stream_;
};
