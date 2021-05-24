#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include "socks/uri.hpp"
#include <string>
#include <memory>
#include <condition_variable>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class WSSession : public std::enable_shared_from_this<WSSession> {
public:
    // Resolver and socket require an io_context
    explicit WSSession(net::io_context& ioc, ssl::context& ctx);

    void setSocksProxy(char const* socks_server) {
        socks_server_ = socks_server;
    }

    void run(char const* host, char const* port, char const* path);

    bool waitUtilConnected(std::chrono::seconds sec);

    void send(const std::string& data);
    void read(std::string* out_data);

private:
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);

    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    void on_ssl_handshake(beast::error_code ec);

    void on_handshake(beast::error_code ec);

    void on_write(beast::error_code ec, std::size_t bytes_transferred);

    void on_read(beast::error_code ec, std::size_t bytes_transferred);

    void on_close(beast::error_code ec);

    void on_socks_proxy_resolve(beast::error_code ec, tcp::resolver::results_type results);

    void on_socks_proxy_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);

    void on_socks_proxy_handshake(beast::error_code ec);

private:
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string port_;
    std::string path_;
    std::string socks_server_;
    socks::uri socks_url_;
    int socks_version_;

    std::mutex cond_mutex_;
    bool connected_ = false;
    std::condition_variable conn_condition_;

};
