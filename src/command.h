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

    // 账户余额和持仓频道
    static Request makeSubscriBebalanceAndPositionChannel();

    // 订单频道
    // inst_type: SPOT：币币; MARGIN：币币杠杆 SWAP：永续合约 FUTURES：交割合约 OPTION：期权 ANY： 全部
    static Request makeSubscribeOrdersChannel(const std::string& inst_type="ANY", const std::string& inst_id="");

    // 交易下单




    // Public
    // 行情频道
    static Request makeSubscriTickersChannel(const std::string& inst_id);


    
    static bool parseReceivedData(const std::string& data, Response* out_resp);



};
