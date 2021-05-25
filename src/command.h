#pragma once

#include <string>
#include <deque>

class Command {
public:
    struct Request {
        std::string id;
        std::string op;
        std::string data;
    };

    struct Response {
        std::string id;
        std::string op;
        int code;
        std::string msg;
        std::string data;
    };

    static Request makeLoginReq(const std::string& api_key, const std::string& passphrase, const std::string& secret);

    static Request makeSubscribeAccountChannel();

    static bool parseReceivedData(const std::string& data, Response* out_resp);

};
