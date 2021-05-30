#include "command.h"
#include "util.h"
#include "user_data.h"
#include "json.h"
#include "crypto/base64.h"
#include <iostream>

extern bool g_show_trades;


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

Command::Request Command::makeSubscribePositionsChannel(const std::string& inst_type, const std::string& inst_id) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "positions", doc.GetAllocator());
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

Command::Request Command::makeSubscriBebalanceAndPositionsChannel() {
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

Command::Request Command::makeOrderReq(const std::string& inst_id, TradeMode trade_mode,
    const OrderData& order_data) {
    rapidjson::Document doc(rapidjson::kObjectType);
    auto id = generateRandomString(10);
    doc.AddMember("id", id, doc.GetAllocator());
    doc.AddMember("op", "order", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    if (!order_data.clordid.empty())
        arg.AddMember("clOrdId", rapidjson::StringRef(order_data.clordid), doc.GetAllocator());

    arg.AddMember("instId", inst_id, doc.GetAllocator());
    
    std::string side_str = toString(order_data.side);
    arg.AddMember("side", side_str, doc.GetAllocator());

    std::string pos_side_str;
    if (order_data.pos_side == OrderPosSide::Long)
        pos_side_str = "long";
    else if (order_data.pos_side == OrderPosSide::Short)
        pos_side_str = "short";
    else
        pos_side_str = "net";
    arg.AddMember("posSide", pos_side_str, doc.GetAllocator());
    
    std::string tdmode;
    if (trade_mode == TradeMode::Cross)
        tdmode = "cross";
    else if (trade_mode == TradeMode::Cash)
        tdmode = "cash";
    else
        tdmode = "isolated";
    arg.AddMember("tdMode", tdmode, doc.GetAllocator());

    std::string order_type_str = order_data.order_type == OrderType::Market ? "market" : "limit";
    arg.AddMember("ordType", order_type_str, doc.GetAllocator());

    if (order_data.order_type == OrderType::Limit)
        arg.AddMember("px", rapidjson::StringRef(order_data.px), doc.GetAllocator());

    arg.AddMember("sz", rapidjson::StringRef(order_data.amount), doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    LOG(info) << "\b\b! order \t" << side_str << " \t" << pos_side_str << " \t"
        << std::left << std::setw(10) << order_data.px << " \t" << order_data.amount;

    Request req;
    req.id = id;
    req.op = "order";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeMultiOrderReq(const std::string& inst_id, TradeMode trade_mode, std::deque<OrderData>& orders) {
    if (orders.size() == 1) {
        auto order = orders.front();
        orders.pop_front();
        return makeOrderReq(inst_id, trade_mode, order);
    }

    rapidjson::Document doc(rapidjson::kObjectType);
    auto id = generateRandomString(10);
    doc.AddMember("id", id, doc.GetAllocator());
    doc.AddMember("op", "batch-orders", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);

    int i = 0;
    while (!orders.empty()) {
        if (++i > 20)
            break;

        auto order = orders.front();
        orders.pop_front();

        rapidjson::Value arg(rapidjson::kObjectType);

        if (!order.clordid.empty())
            arg.AddMember("clOrdId", order.clordid, doc.GetAllocator());

        arg.AddMember("instId", inst_id, doc.GetAllocator());

        std::string side_str = toString(order.side);
        arg.AddMember("side", side_str, doc.GetAllocator());

        std::string pos_side_str;
        if (order.pos_side == OrderPosSide::Long)
            pos_side_str = "long";
        else if (order.pos_side == OrderPosSide::Short)
            pos_side_str = "short";
        else
            pos_side_str = "net";
        arg.AddMember("posSide", pos_side_str, doc.GetAllocator());

        std::string tdmode;
        if (trade_mode == TradeMode::Cross)
            tdmode = "cross";
        else if (trade_mode == TradeMode::Cash)
            tdmode = "cash";
        else
            tdmode = "isolated";
        arg.AddMember("tdMode", tdmode, doc.GetAllocator());

        std::string order_type_str = order.order_type == OrderType::Market ? "market" : "limit";
        arg.AddMember("ordType", order_type_str, doc.GetAllocator());

        if (order.order_type == OrderType::Limit)
            arg.AddMember("px", order.px, doc.GetAllocator());

        arg.AddMember("sz", order.amount, doc.GetAllocator());

        args.PushBack(arg, doc.GetAllocator());

        LOG(info) << "\b\b! order \t" << side_str << " \t" << pos_side_str
            << " \t" << std::left << std::setw(10) << order.px << " \t" << order.amount;
    }

    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.id = id;
    req.op = "batch-orders";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeCancelOrderReq(const std::string& inst_id, const std::string& cliordid, const std::string& ordid) {
    rapidjson::Document doc(rapidjson::kObjectType);
    auto id = generateRandomString(10);
    doc.AddMember("id", id, doc.GetAllocator());
    doc.AddMember("op", "cancel-order", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("instId", inst_id, doc.GetAllocator());
    if (!ordid.empty())
        arg.AddMember("ordId", rapidjson::StringRef(ordid), doc.GetAllocator());
    if (!cliordid.empty())
        arg.AddMember("clOrdId", rapidjson::StringRef(cliordid), doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.id = id;
    req.op = "cancel-order";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeCancelMultiOrderReq(const std::string& inst_id, std::deque<std::string>& cliordids) {
    if (cliordids.size() == 1) {
        auto cliordid = cliordids.front();
        cliordids.pop_front();
        return makeCancelOrderReq(inst_id, cliordid, "");
    }

    rapidjson::Document doc(rapidjson::kObjectType);
    auto id = generateRandomString(10);
    doc.AddMember("id", id, doc.GetAllocator());
    doc.AddMember("op", "batch-cancel-orders", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);

    int i = 0;
    while (!cliordids.empty()) {
        if (++i > 20)
            break;

        auto cliordid = cliordids.front();
        cliordids.pop_front();

        rapidjson::Value arg(rapidjson::kObjectType);
        arg.AddMember("instId", inst_id, doc.GetAllocator());
        arg.AddMember("clOrdId", cliordid, doc.GetAllocator());
        args.PushBack(arg, doc.GetAllocator());
    }

    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.id = id;
    req.op = "batch-cancel-orders";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeSubscribeInstrumentsChannel(const std::string& inst_type) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "instruments", doc.GetAllocator());
    arg.AddMember("instType", inst_type, doc.GetAllocator());

    args.PushBack(arg, doc.GetAllocator());
    doc.AddMember("args", args, doc.GetAllocator());

    Request req;
    req.op = "subscribe";
    req.data = toString(doc);
    return req;
}

Command::Request Command::makeSubscribeTickersChannel(const std::string& inst_id) {
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

Command::Request Command::makeSubscribeTradesChannel(const std::string& inst_id) {
    rapidjson::Document doc(rapidjson::kObjectType);
    doc.AddMember("op", "subscribe", doc.GetAllocator());

    rapidjson::Value args(rapidjson::kArrayType);
    rapidjson::Value arg(rapidjson::kObjectType);

    arg.AddMember("channel", "trades", doc.GetAllocator());
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

            LOG(debug) << "resp. op=" << resp.op << ", code=" << resp.code << ", msg=" << resp.msg << ", data=" << resp.data;
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
            if (doc.HasMember("msg"))
                resp.msg = doc["msg"].GetString();

            LOG(debug) << "resp. id=" << resp.id << ", op=" << resp.op << ", code=" << resp.code << ", msg=" << resp.msg << ", data=" << resp.data;

            if (resp.op == "order" || resp.op == "batch-orders") {
                if (resp.code == 1 || resp.code == 2) {
                   for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        std::string clordid = (*itr)["clOrdId"].GetString();
                        auto scode = strtol((*itr)["sCode"].GetString(), nullptr, 0);
                        if (!clordid.empty() && scode != 0) {
                            g_user_data.lock();
                            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

                            for (auto& grid : g_user_data.grid_strategy_.grids) {
                                auto orders_arr = { &grid.long_orders, &grid.short_orders };
                                for (auto ordersq : orders_arr) {
                                    for (auto& order : ordersq->orders) {
                                        if (order.order_data.clordid == clordid) {
                                            order.order_status = OrderStatus::Canceled;
                                            order.order_data.clordid.clear();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    g_user_data.updateGrid();
                }
            }

            *out_resp = std::move(resp);
            return true;
        } else if (doc.HasMember("data")) {
            if (doc.HasMember("arg")) {
                std::string channel = doc["arg"]["channel"].GetString();

                if (channel == "account") {
                    g_user_data.lock();
                    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
                    g_user_data.balance_.inited = true;

                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        auto& bal = (*itr)["details"];
                        for (auto itrbal = bal.Begin(); itrbal != bal.End(); ++itrbal) {
                            auto& val = *itrbal;
                            std::string ccy = val["ccy"].GetString();
                            UserData::Balance::BalVal balval;
                            balval.eq = val["eq"].GetString();
                            balval.upl = val["upl"].GetString();
                            balval.avail_eq = val["availEq"].GetString();
                            balval.cash_bal = val["cashBal"].GetString();
                            g_user_data.balance_.balval[ccy] = balval;
                        }
                    }
                } else if (channel == "positions") {
                    g_user_data.lock();
                    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

                    for (auto positr = doc["data"].Begin(); positr != doc["data"].End(); ++positr) {
                        UserData::Position::PosData data;

                        data.pos_id = (*positr)["posId"].GetString();
                        data.inst_id = (*positr)["instId"].GetString();
                        data.inst_type = (*positr)["instType"].GetString();

                        data.pos_side = (*positr)["posSide"].GetString();
                        data.avg_px = (*positr)["avgPx"].GetString();
                        data.pos = (*positr)["pos"].GetString();
                        data.ccy = (*positr)["ccy"].GetString();
                        auto utime = (*positr)["uTime"].GetString();
                        data.utime_msec = std::strtoull(utime, nullptr, 0);

                        if (data.pos == "0")
                            g_user_data.position_.posval.erase(data.pos_id);
                        else
                            g_user_data.position_.posval[data.pos_id] = std::move(data);
                    }
                } else if (channel == "balance_and_position") {
                    g_user_data.lock();
                    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });
                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        std::string event_type = (*itr)["eventType"].GetString();

                        if (itr->HasMember("balData")) {
                            g_user_data.balance_.inited = true;
                            auto& bal_data = (*itr)["balData"];
                            for (auto balitr = bal_data.Begin(); balitr != bal_data.End(); ++balitr) {
                                std::string ccy = (*balitr)["ccy"].GetString();
                                UserData::Balance::BalVal balval;
                                balval.cash_bal = (*balitr)["cashBal"].GetString();
                                g_user_data.balance_.balval[ccy] = balval;
                            }
                        } 

                        if (itr->HasMember("posData")) {
                            auto& pos_data = (*itr)["posData"];
                            for (auto positr = pos_data.Begin(); positr != pos_data.End(); ++positr) {
                                UserData::Position::PosData data;

                                data.pos_id = (*positr)["posId"].GetString();
                                data.inst_id = (*positr)["instId"].GetString();
                                data.inst_type = (*positr)["instType"].GetString();

                                data.pos_side = (*positr)["posSide"].GetString();
                                data.avg_px = (*positr)["avgPx"].GetString();
                                data.pos = (*positr)["pos"].GetString();
                                data.ccy = (*positr)["ccy"].GetString();
                                auto utime = (*positr)["uTime"].GetString();
                                data.utime_msec = std::strtoull(utime, nullptr, 0);

                                if (data.pos == "0")
                                    g_user_data.position_.posval.erase(data.pos_id);
                                else
                                    g_user_data.position_.posval[data.pos_id] = std::move(data);
                            }
                        }
                    }
                } else if (channel == "orders") {
                    std::ostringstream o;
                    o << "=====Orders=====" << std::endl;

                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        std::string inst_type = (*itr)["instType"].GetString();
                        std::string inst_id = (*itr)["instId"].GetString();
                        std::string ord_id = (*itr)["ordId"].GetString();
                        std::string clordid = (*itr)["clOrdId"].GetString();
                        std::string side = (*itr)["side"].GetString(); // buy sell
                        std::string pos_side = (*itr)["posSide"].GetString(); // 持仓方向 long short net
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

                        uint64_t utime = std::strtoull((*itr)["uTime"].GetString(), nullptr, 0);
                        uint64_t ctime = std::strtoull((*itr)["cTime"].GetString(), nullptr, 0);

                        if (!clordid.empty()) {
                            g_user_data.lock();
                            auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

                            for (auto& grid : g_user_data.grid_strategy_.grids) {
                                auto orders_arr = { &grid.long_orders, &grid.short_orders };
                                for (auto ordersq : orders_arr) {
                                    for (auto& order : ordersq->orders) {
                                        if (order.order_data.clordid == clordid) {
                                            if (state == "canceled") {
                                                order.order_status = OrderStatus::Canceled;
                                                order.order_data.clordid.clear();
                                            } else if (state == "filled") {
                                                order.order_status = OrderStatus::Filled;
                                                order.fill_px = fill_px;
                                            } else if (state == "partially_filled") {
                                                order.order_status = OrderStatus::PartiallyFilled;
                                            } else if (state == "live") {
                                                order.order_status = OrderStatus::Live;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        o << "  - " << ord_id << " " << inst_id << "  " << inst_type << " " << state << " \t" << toTimeStr(utime) << std::endl;
                        if (state == "live")
                            o << "    order: \t" << sz << " \t" << std::left << std::setw(10) << px << " \t" << lever << "x" << std::endl;
                        else if (state == "filled" || state == "partially_filled") {
                            o << "    filled: \t" << fill_sz << " \t" << fill_px << " \t" << lever << "x" << std::endl;

                            std::ostringstream ofilledlog;
                            ofilledlog << "\b\b! " << state << " \t" << side << " \t" << pos_side
                                << " \t" << std::left << std::setw(10) << px << " \t" << fill_sz << " \t" << lever << "x" << " \t" << toTimeStr(utime);
                            g_user_data.grid_strategy_.filled_history_log_.push_front(ofilledlog.str());
                            if (g_user_data.grid_strategy_.filled_history_log_.size() > 100)
                                g_user_data.grid_strategy_.filled_history_log_.pop_back();
                            std::cout << ofilledlog.str() << std::endl;
                        }
                        o << "    total: \t" << acc_fill_sz << " \t" << avg_px << std::endl;
                    }
                    LOG(debug) << o.str();
                    g_user_data.updateGrid();
                } else if (channel == "trades") {
                    g_user_data.lock();
                    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        UserData::PublicTradesInfo::Info info;
                        info.inst_id = (*itr)["instId"].GetString();
                        info.trade_id = (*itr)["tradeId"].GetString();
                        info.px = (*itr)["px"].GetString();
                        info.sz = (*itr)["sz"].GetString();
                        info.pos_side = (*itr)["side"].GetString();
                        info.ts = std::strtoull((*itr)["ts"].GetString(), nullptr, 0);

                        if (g_show_trades)
                            std::cout << "  - " << info.inst_id << " \t" << info.pos_side << " \t"
                                << info.sz << " \t" << info.px << "  \t" << toTimeStr(info.ts) << std::endl;

                        g_user_data.public_trades_info_.trades_data[info.inst_id] = std::move(info);
                    }
                } else if (channel == "instruments") {
                    g_user_data.lock();
                    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

                    for (auto itr = doc["data"].Begin(); itr != doc["data"].End(); ++itr) {
                        UserData::ProductInfo::Info info;
                        info.inst_id = (*itr)["instId"].GetString();
                        info.inst_type = (*itr)["instType"].GetString();
                        info.lot_sz = (*itr)["lotSz"].GetString(); // 下单数量精度
                        info.min_sz = (*itr)["minSz"].GetString(); // 最小下单数量
                        info.tick_sz = (*itr)["tickSz"].GetString(); // 最小下单价格
                        info.ct_val = (*itr)["ctVal"].GetString(); // 合约面值
                        info.ct_multi = (*itr)["ctMult"].GetString(); // 合约乘数
                        info.settle_ccy = (*itr)["settleCcy"].GetString();

                        g_user_data.public_product_info_.data[info.inst_id] = std::move(info);
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LOG(error) << "pase failed! " << e.what() << "\ndata=" << data;
    }
    return false;    
}
