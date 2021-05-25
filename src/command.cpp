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
                            auto& bal_data = (*itr)["balData"];
                            for (auto balitr = bal_data.Begin(); balitr != bal_data.End(); ++balitr) {
                                std::string ccy = (*balitr)["ccy"].GetString();
                                int cash_bal = std::strtol((*balitr)["cashBal"].GetString(), nullptr, 0);

                                LOG(debug) << "CCY: " << ccy << "\tcash:" << cash_bal;
                            }
                        } 

                        if (itr->HasMember("posData")) {
                            auto& pos_data = (*itr)["posData"];
                            for (auto positr = pos_data.Begin(); positr != pos_data.End(); ++positr) {
                                std::string pos_id = (*positr)["posId"].GetString();
                                std::string trade_id = (*positr)["tradeId"].GetString();
                                std::string inst_id = (*positr)["instId"].GetString();
                                std::string inst_type = (*positr)["instType"].GetString();
                                std::string pos_side = (*positr)["posSide"].GetString();
                                int pos = std::strtol((*positr)["pos"].GetString(), nullptr, 0);
                                std::string ccy = (*positr)["ccy"].GetString();

                                LOG(debug) << "POS: " << pos_id << "\ttrade_id:" << trade_id 
                                    << "\tinst_id:" << inst_id << "\tinst_type:" << inst_type
                                    << "\tside:" << pos_side << "\tpos:" << pos << "\tccy:" << ccy;
                            }
                        }
                    }
                }
            }            
        }
    } catch (const std::exception& e) {
        LOG(error) << "pase failed! " << e.what() << "\ndata=" << data;
    }
    return false;    
}
