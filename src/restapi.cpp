#include "restapi.h"
#include "global.h"
#include "crypto/base64.h"
#include "util.h"
#include "json.h"
#include <deque>
#include <iomanip>

#define SET_LEVERAGE_PATH   "/api/v5/account/set-leverage"
#define GET_LEVERAGE_PATH   "/api/v5/account/leverage-info"

static std::string url_encode(const string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        string::value_type c = (*i);

        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }
    return escaped.str();
}

static std::string makePath(const std::string& path, const std::deque<std::pair<std::string, std::string> >& params) {
    if (params.empty())
        return path;

    std::string o;
    for (auto& v : params) {
        if (!o.empty())
            o += "&";
        else
            o += "?";
        o += url_encode(v.first);
        o += "=";
        o += url_encode(v.second);
    }
    return path + o;
}


RestApi::RestApi(net::io_context& ioc,
    const std::string& host, const std::string& port,
    const std::string socks_proxy)
: ioc_(ioc),
    host_(host),
    port_(port),
    socks_proxy_(socks_proxy) {
}

RestApi::~RestApi() {
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

bool RestApi::getLeverage(int* lever) {
    std::deque<std::pair<std::string, std::string> > params;
    params.emplace_back("instId", g_ticket);
    params.emplace_back("mgnMode", "cross");

    resp_type resp;
    if (sendCmd("GET", makePath(GET_LEVERAGE_PATH, params), "", &resp)) {


        return true;
    }
    return false;
}

bool RestApi::sendCmd(const string& verbstr, const std::string& path, const std::string& reqdata, resp_type* resp) {
    try {
        http::verb httpverb = http::verb::get;
        if (verbstr == "POST")
            httpverb = http::verb::post;
        else if (verbstr == "PUT")
            httpverb = http::verb::put;
        else if (verbstr == "DELETE")
            httpverb = http::verb::delete_;
        int version = 11;

        http::request<http::string_body> req{ httpverb, path, version, reqdata };
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
        auto sign = BASE64Encode(calcHmacSHA256(timestamp + verbstr + path + reqdata, g_secret));

        req.set("OK-ACCESS-SIGN", sign);
        req.set("OK-ACCESS-TIMESTAMP", timestamp);

        ssl::context ctx{ ssl::context::tlsv12_client };
        auto http_session = std::make_shared<HttpSession>(ioc_, ctx, host_, port_);
        http_session->send(req);
        return http_session->read(*resp);
    } catch (const std::exception& e) {
        LOG(error) << "exception: " << e.what();
    }
    return false;
}
