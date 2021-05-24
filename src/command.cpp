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
std::string Command::makeLoginReq(const std::string& api_key, const std::string& passphrase, const std::string& secret) {
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

    return toString(doc);
}

std::string Command::makeSubscribeAccountChannel() {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "account", doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    return toString(doc);
}
