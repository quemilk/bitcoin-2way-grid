#include "restapi.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;


RestApi::RestApi(const std::string& host)
: host_(host) {

}

void RestApi::setLeverage(int lever) {

}

int RestApi::getLeverage() {

}

std::string RestApi::call(const std::string& verb,const std::string& reqdata) {
    try {
        net::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);

        ctx.set_verify_mode(ssl::verify_peer);

        tcp::resolver resolver(ioc);
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host)) {
            beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
            throw beast::system_error{ ec };
        }

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        http::verb httpverb = http::verb::get;
        if (verb == "POST")
            httpverb = http:verb::post;
        else if (verb == "PUT")
            httpverb = http:verb::put;
        else if (verb == "DELETE")
            httpverb = http:verb::delete_;

        http::request<http::string_body> req{ httpverb, target, version, reqdata };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, ibitcoin2waygrid);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(stream, buffer, res);
        beast::error_code ec;
        stream.shutdown(ec);


    } catch (...) {

    }
}
