#pragma once

#include <string>

class Command {
public:
    struct Request {
        std::string op;
        std::string data;
    };

    static Request makeLoginReq(const std::string& api_key, const std::string& passphrase, const std::string& secret);

    static Request makeSubscribeAccountChannel();

    static void parseReceivedData(const std::string& data, bool* is_resp);

};
