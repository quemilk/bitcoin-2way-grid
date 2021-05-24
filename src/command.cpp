#include "command.h"
#include "json.h"
#include "crypto/base64.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string>
#include <string_view>
#include <array>

static std::string toString(rapidjson::Value& v) {
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    v.Accept(writer);
    return strbuf.GetString();
}

static std::string calcHmacSHA256(std::string_view decodedKey, std::string_view msg) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
    unsigned int hashLen;

    HMAC(
        EVP_sha256(),
        decodedKey.data(),
        static_cast<int>(decodedKey.size()),
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
    doc["op"] = "login";

    rapidjson::Value arg;
    doc["args"].PushBack(arg, doc.GetAllocator());

    auto timestamp = std::to_string(time(nullptr));
    auto sign = BASE64Encode(calcHmacSHA256(timestamp + "GET" + "/users/self/verify", secret));

    arg["apiKey"].SetString(rapidjson::StringRef(api_key), doc.GetAllocator());
    arg["passphrase"].SetString(rapidjson::StringRef(passphrase), doc.GetAllocator());
    arg["timestamp"].SetString(rapidjson::StringRef(timestamp), doc.GetAllocator());
    arg["sign"].SetString(rapidjson::StringRef(sign), doc.GetAllocator());

    return toString(doc);
}
