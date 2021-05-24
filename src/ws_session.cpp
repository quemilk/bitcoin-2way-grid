#include "ws_session.h"
#include "logger.h"
#include "socks/handshake.hpp"
#include <iostream>


// Report a failure
void fail(beast::error_code ec, char const* what) {
    auto msg = ec.message();
    LOG(error) << what << ": " << msg;
}


WSSession::WSSession(net::io_context& ioc, ssl::context& ctx)
    : resolver_(net::make_strand(ioc))
    , ws_(net::make_strand(ioc), ctx) {
}

// Start the asynchronous operation
void WSSession::run(char const* host, char const* port, char const* path) {
    // Save these for later
    host_ = host;
    port_ = port;
    path_ = path;

    if (!socks_server_.empty()) {
        if (!socks_url_.parse(socks_server_)) {
            std::cerr << "parse socks url error\n";
            return;
        }
        // socks handshake.
        socks_version_ = socks_url_.scheme() == "socks4" ? 4 : 0;
        socks_version_ = socks_url_.scheme() == "socks5" ? 5 : socks_version_;
        if (socks_version_ == 0) {
            std::cerr << "incorrect socks version\n";
            return;
        }
        // Look up the domain name
        resolver_.async_resolve(
            std::string(socks_url_.host()),
            std::string(socks_url_.port()),
            beast::bind_front_handler(
                &WSSession::on_socks_proxy_resolve,
                shared_from_this()));
        return;
    }

    // Look up the domain name
    resolver_.async_resolve(
        host,
        port,
        beast::bind_front_handler(
            &WSSession::on_resolve,
            shared_from_this()));
}

void WSSession::on_socks_proxy_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec)
        return fail(ec, "resolve");

    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &WSSession::on_socks_proxy_connect,
            shared_from_this()));
}

void WSSession::on_socks_proxy_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
    if (ec)
        return fail(ec, "proxy_connect");

     if (socks_version_ == 4)
        socks::async_handshake_v4(
            beast::get_lowest_layer(ws_),
            host_,
            static_cast<unsigned short>(std::atoi(port_.c_str())),
            std::string(socks_url_.username()),
            beast::bind_front_handler(
                &WSSession::on_socks_proxy_handshake,
                shared_from_this()));
    else
        socks::async_handshake_v5(
            beast::get_lowest_layer(ws_),
            host_,
            static_cast<unsigned short>(std::atoi(port_.c_str())),
            std::string(socks_url_.username()),
            std::string(socks_url_.password()),
            true,
            beast::bind_front_handler(
                &WSSession::on_socks_proxy_handshake,
                shared_from_this()));
}

void WSSession::on_socks_proxy_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "proxy_handshake");

    on_connect(ec, tcp::resolver::results_type::endpoint_type());
}

void WSSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec)
        return fail(ec, "resolve");

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &WSSession::on_connect,
            shared_from_this()));
}

void  WSSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
    if (ec)
        return fail(ec, "connect");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + port_;

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(
        ws_.next_layer().native_handle(),
        host_.c_str())) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category());
        return fail(ec, "connect");
    }

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(
            &WSSession::on_ssl_handshake,
            shared_from_this()));
}

void WSSession::on_ssl_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "ssl_handshake");

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::user_agent, "ibitcoin2waygrid");
        }));

    // Perform the websocket handshake
    ws_.async_handshake(host_, path_,
        beast::bind_front_handler(
            &WSSession::on_handshake,
            shared_from_this()));
}

void WSSession::on_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "handshake");

    //// Send the message
    //ws_.async_write(
    //    net::buffer(text_),
    //    beast::bind_front_handler(
    //        &WSSession::on_write,
    //        shared_from_this()));
}

void WSSession::on_write(
    beast::error_code ec,
    std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "write");

    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &WSSession::on_read,
            shared_from_this()));
}

void WSSession::on_read(
    beast::error_code ec,
    std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec)
        return fail(ec, "read");

    // Close the WebSocket connection
    ws_.async_close(websocket::close_code::normal,
        beast::bind_front_handler(
            &WSSession::on_close,
            shared_from_this()));
}

void WSSession::on_close(beast::error_code ec) {
    if (ec)
        return fail(ec, "close");

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    // std::cout << beast::make_printable(buffer_.data()) << std::endl;
}