#pragma once

#include <string>
#include <deque>

class Command {
public:
    struct Request {
        std::string op;
        std::string data;
    };

    struct Response {
        std::string op;
        int code;
        std::string msg;
    };

    static Request makeLoginReq(const std::string& api_key, const std::string& passphrase, const std::string& secret);

    static Request makeSubscribeAccountChannel();

    static void parseReceivedData(const std::string& data, std::deque<Response>* out_resp);

};
