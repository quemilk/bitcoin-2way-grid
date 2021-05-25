#include "command.h"
#include "json.h"
#include "crypto/base64.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string>
#include <array>

static std::string toString(rapidjson::Value& v) {
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    v.Accept(writer);
    return strbuf.GetString();
}

static std::string calcHmacSHA256(const std::string& msg, const std::string& decoded_key) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
    unsigned int hashLen;

    HMAC(
        EVP_sha256(),
        decoded_key.data(),
        static_cast<int>(decoded_key.size()),
        reinterpret_cast<unsigned char const*>(msg.data()),
        static_cast<int>(msg.size()),
        hash.data(),
        &hashLen
    );

    return std::string{ reinterpret_cast<char const*>(hash.data()), hashLen };
}

static std::string toTimeStr(int time_msec) {
    struct tm tstruct;
    char buf[80];
    time_t now = time_msec / 1000;
    tstruct = *localtime(&now);
    strftime(buf, _countof(buf), "%m-%d %H:%M:%S", &tstruct);

    return buf;
}


/*{
 "op": "login",
 "args":
  [
     {
       "apiKey"    : "<api_key>",
       "passphrase" :"<passphrase>",
       "timestamp" :"<timestamp>",
       "sign" :"<sign>" 
      }
   ]
}
sign=CryptoJS.enc.Base64.Stringify(CryptoJS.HmacSHA256(timestamp +'GET'+ '/users/self/verify', secret))
*/
Command::Request Command::makeLoginReq(const std::string& api_key, const std::string& passphrase, const std::string& secret) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "login", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    auto timestamp = std::to_string(time(nullptr));
    auto sign = BASE64Encode(calcHmacSHA256(timestamp + "GET" + "/users/self/verify", secret));

    arg.AddMember("apiKey", rapidjson::StringRef(api_key), doc.GetAllocator());
    arg.AddMember("passphrase", rapidjson::StringRef(passphrase), doc.GetAllocator());
    arg.AddMember("timestamp", rapidjson::StringRef(timestamp), doc.GetAllocator());
    arg.AddMember("sign", rapidjson::StringRef(sign), doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "login";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeSubscribeAccountChannel() {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "account", doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "subscribe";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeSubscriBebalanceAndPositionChannel() {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "balance_and_position", doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "subscribe";
    req.data = toString(doc);
    return req;
}


Command::Request Command::makeSubscribeOrdersChannel(const std::string& inst_type, const std::string& inst_id) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "orders", doc.GetAllocator());
    arg.AddMember("instType", inst_type, doc.GetAllocator());
    if (!inst_id.empty())
        arg.AddMember("instId", inst_id, doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "subscribe";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeSubscriTickersChannel(const std::string& inst_id) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "tickers", doc.GetAllocator());
    arg.AddMember("instId", inst_id, doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "subscribe";
    req.data = toString(doc);
    return req;
}


bool Command::parseReceivedData(const std::string& data, Response* out_resp) {
    try {
        rapidjson::Document doc(rapidjson::kObjectType);
        doc.Parse<0>(data);

        if (doc.HasMember("event")) {
            std::string event = doc["event"].GetString();

            Response resp;
            resp.data = data;
            resp.op = event;
            resp.code = 0;
            if (doc.HasMember("id"))
                resp.id = doc["id"].GetString();

            if (doc.HasMember("code"))
                resp.code = std::strtol(doc["code"].GetString(), nullptr, 0);
            if (event == "error" && 0 == resp.code)
                resp.code = -1;
            if (doc.HasMember("msg"))
                resp.msg = doc["msg"].GetString();
            
            *out_resp = std::move(resp);
            return true;
        } else if (doc.HasMember("op")) {
            Response resp;
            resp.data = data;
            resp.op = doc["op"].GetString();
            resp.code = 0;
            if (doc.HasMember("id"))
                resp.id = doc["id"].GetString();
            if (doc.HasMember("code"))
                resp.code = std::strtol(doc["code"].GetString(), nullptr, 0);
            *out_resp = std::move(resp);
            return true;
        } else if (doc.HasMember("data")) {
            if (doc.HasMember("arg")) {
                std::string channel = doc["arg"]["channel"].GetString();
                if (channel == "balance_and_position") {
                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        std::string event_type = (*itr)["eventType"].GetString();

                        if (itr->HasMember("balData")) {
                            std::ostringstream o;
                            o << "=====Balance=====" << std::endl;

                            auto& bal_data = (*itr)["balData"];
                            for (auto balitr = bal_data.Begin(); balitr != bal_data.End(); ++balitr) {
                                std::string ccy = (*balitr)["ccy"].GetString();
                                int cash_bal = std::strtol((*balitr)["cashBal"].GetString(), nullptr, 0);

                                o << "  " << ccy << " \tcash: " << cash_bal << std::endl;
                            }
                            LOG(debug) << o.str();
                        } 

                        if (itr->HasMember("posData")) {
                            std::ostringstream o;
                            o << "=====Position=====" << std::endl;

                            auto& pos_data = (*itr)["posData"];
                            for (auto positr = pos_data.Begin(); positr != pos_data.End(); ++positr) {
                                std::string pos_id = (*positr)["posId"].GetString();
                                std::string trade_id = (*positr)["tradeId"].GetString();
                                std::string inst_id = (*positr)["instId"].GetString();
                                std::string inst_type = (*positr)["instType"].GetString();
                                std::string pos_side = (*positr)["posSide"].GetString();
                                std::string avg_px = (*positr)["avgPx"].GetString();
                                int pos = std::strtol((*positr)["pos"].GetString(), nullptr, 0);
                                std::string ccy = (*positr)["ccy"].GetString();
                                int utime = std::strtol((*positr)["uTime"].GetString(), nullptr, 0); // 仓位信息更新时间
                                o << "  - " << pos_id  << " " << inst_id << "  " << inst_type << std::endl
                                    << "    trade_id:" << trade_id << " \t" << pos_side << " \t" << pos << " \t" << avg_px << " \t" << ccy << std::endl;
                            }
                            LOG(debug) << o.str();
                        }
                    }
                } else if (channel == "orders") {
                    std::ostringstream o;
                    o << "=====Orders=====" << std::endl;

                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        std::string inst_type = (*itr)["instType"].GetString();
                        std::string inst_id = (*itr)["instId"].GetString();
                        std::string ord_id = (*itr)["ordId"].GetString();
                        std::string side = (*itr)["side"].GetString(); // buy sell
                        std::string posSide = (*itr)["posSide"].GetString(); // 持仓方向 long short net
                        std::string px = (*itr)["px"].GetString(); // 委托价格
                        std::string sz = (*itr)["sz"].GetString(); // 原始委托数量
                        std::string fill_px = (*itr)["fillPx"].GetString(); // 最新成交价格
                        std::string fill_sz = (*itr)["fillSz"].GetString(); // 最新成交数量
                        std::string avg_px = (*itr)["avgPx"].GetString(); // 成交均价
                        std::string acc_fill_sz = (*itr)["accFillSz"].GetString(); // 累计成交数量

                        std::string state = (*itr)["state"].GetString(); // 订单状态 canceled：撤单成功 live：等待成交 partially_filled： 部分成交 filled：完全成交
                        std::string category = (*itr)["category"].GetString(); // 订单种类分类 normal：普通委托订单种类 twap：TWAP订单种类 adl：ADL订单种类 full_liquidation：爆仓订单种类 partial_liquidation：减仓订单种类

                        std::string lever = (*itr)["lever"].GetString(); // 杠杆倍数
                        std::string fee = (*itr)["fee"].GetString(); // 订单交易手续费
                        std::string pnl = (*itr)["pnl"].GetString(); // 收益

                        int utime = std::strtol((*itr)["uTime"].GetString(), nullptr, 0); // 订单更新时间
                        int ctime = std::strtol((*itr)["cTime"].GetString(), nullptr, 0); // 订单更新时间
    
                        o << "  - " << ord_id << " " << inst_id << "  " << inst_type << " " << state << "\t" << toTimeStr(utime) << std::endl;
                        if (state == "live")
                            o << "    order: \t" << sz << " \t" << px << " \t" << lever << "x" << std::endl;
                        else if (state == "filled" || state == "partially_filled")
                            o << "    filled: \t" << fill_sz << " \t" << fill_px << " \t" << lever << "x" << std::endl;
                        o << "    total: \t" << acc_fill_sz << " \t" << avg_px << std::endl;
                    }
                    LOG(debug) << o.str();
                }
            }
        }
    } catch (const std::exception& e) {
        LOG(error) << "pase failed! " << e.what() << "\ndata=" << data;
    }
    return false;    
}
