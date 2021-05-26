﻿#pragma once

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
    enum class OrderSide { Buy, Sell };
    enum class OrderType {
        Market, // 市价单
        Limit // 限价单
    };
    enum class TradeMode {
        Isolated, //逐仓
        Cross,  // 全仓
        Cash //现金
    };
    static Request makeOrderReq(const std::string& inst_id, OrderType order_type, TradeMode trade_mode,
        OrderSide side, const std::string& px, const std::string& amount);



    // Public
    // 产品频道
    static Request makeSubscribeInstrumentsChannel(const std::string& inst_type);
    // 行情频道
    static Request makeSubscribeTickersChannel(const std::string& inst_id);
    // 交易频道
    static Request makeSubscribeTradesChannel(const std::string& inst_id);

    
    static bool parseReceivedData(const std::string& data, Response* out_resp);



};
