#include "restapi.h"
#include "global.h"
#include "crypto/base64.h"
#include "util.h"
#include "json.h"


#define SET_LEVERAGE_PATH   "/api/v5/account/set-leverage"
#define GET_LEVERAGE_PATH   "/api/v5/account/leverage-info"



RestApi::RestApi(net::io_context& ioc,
    const std::string& host, const std::string& port,
    const std::string socks_proxy) {
    // TODO
}

/*
{
    "instId":"BTC-USDT",
    "lever" : "5",
    "mgnMode" : "isolated"
} */
bool RestApi::setLeverage(int lever) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("instId", g_ticket, doc.GetAllocator());
    doc.AddMember("lever", std::to_string(lever), doc.GetAllocator());
    doc.AddMember("mgnMode", "cross", doc.GetAllocator());
    std::string reqdata = toString(doc);
    //auto respdata = call("POST", SET_LEVERAGE_PATH, reqdata);


    return true;
}

int RestApi::getLeverage() {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("instId", g_ticket, doc.GetAllocator());
    doc.AddMember("mgnMode", "cross", doc.GetAllocator());
    std::string reqdata = toString(doc);
    //auto respdata = call("POST", SET_LEVERAGE_PATH, reqdata);



    return 0;
}

/*
std::string RestApi::call(const std::string& verb, const std::string& target, const std::string& reqdata) {
    std::string respdata;
    try {
        net::io_context ioc;
        ssl::context ctx(ssl::context::tlsv12_client);

        tcp::resolver resolver(ioc);

        if (!socks_server_.empty()) {
            if (!socks_url_.parse(socks_server_)) {
                throw std::runtime_error("parse socks url error");
            }

            socks_version_ = socks_url_.scheme() == "socks4" ? 4 : 0;
            socks_version_ = socks_url_.scheme() == "socks5" ? 5 : socks_version_;
            if (socks_version_ == 0) {
                throw std::runtime_error("incorrect socks version");
            }

            auto const results = resolver.resolve(
                std::string(socks_url_.host()),
                std::string(socks_url_.port()));


        }

        ctx.set_verify_mode(ssl::verify_peer);

        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str())) {
            beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
            throw beast::system_error{ ec };
        }

        // Look up the domain name
        auto const results = resolver.resolve(host_, port_);

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(stream).connect(results);

        // Perform the SSL handshake
        stream.handshake(ssl::stream_base::client);

        http::verb httpverb = http::verb::get;
        if (verb == "POST")
            httpverb = http::verb::post;
        else if (verb == "PUT")
            httpverb = http::verb::put;
        else if (verb == "DELETE")
            httpverb = http::verb::delete_;

        int version = 11;
        http::request<http::string_body> req { httpverb, target, version, reqdata };
        req.set(http::field::host, host_);
        req.set(http::field::user_agent, "ibitcoin2waygrid");

        req.set(http::field::content_type, "application/json");
        req.set("OK-ACCESS-KEY", g_api_key);
        req.set("OK-ACCESS-PASSPHRASE", g_passphrase);

        if (host_ == SIMU_REST_API_HOST)
            req.set("x-simulated-trading", std::to_string(1));

        time_t now;
        std::time(&now);
        char buf[64];
        strftime(buf, sizeof buf, "%FT%TZ", std::gmtime(&now));
        std::string timestamp = buf;
        auto sign = BASE64Encode(calcHmacSHA256(timestamp + verb + target + reqdata, g_secret));

        req.set("OK-ACCESS-SIGN", sign);
        req.set("OK-ACCESS-TIMESTAMP", timestamp);

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;

        http::read(stream, buffer, res);

        respdata = beast::buffers_to_string(buffer.data());

        beast::error_code ec;
        stream.shutdown(ec);


    } catch (...) {

    }
    return respdata;
}
*/