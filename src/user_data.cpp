#include "user_data.h"
#include "private_channel.h"
#include "logger.h"
#include "command.h"
#include <deque>

extern std::string g_ticket;
extern std::shared_ptr<PrivateChannel> g_private_channel;

UserData g_user_data;

static std::string floatToString(float f, const std::string& tick_sz) {
    std::vector<std::string> v;
    splitString(tick_sz, v, '.');

    if (v.size() == 1) {
        auto l = strtol(v[0].c_str(), nullptr, 0);
        if (l)
            return std::to_string((int)(f / l * l));
    } else if (v.size() == 2) {
        auto l = strtol(v[0].c_str(), nullptr, 0);
        auto d = strtol(v[1].c_str(), nullptr, 0);
        int power = 1;
        int digit = v[1].size();
        for (int i = 0; i < digit; ++i)
            power *= 10;
        int r = (int)(f * power / (l * power + d));
        auto ipart = std::to_string((int)(r / power));
        auto fpart = std::to_string((int)(r % power));
        std::string o = ipart;
        if (digit > 0) {
            o += ".";
            for (int i = 0; i < digit - (int)fpart.size(); ++i)
                o += "0";
            o += fpart;
        }
        return o;
    } else {
        assert(false);
    }
    return std::string();
}


void UserData::startGrid(float injected_cash, int grid_count, float step_ratio) {
    if (grid_count <= 0 || step_ratio <= 0 || step_ratio >= 1.0f) {
        LOG(error) << "invalid param!";
        return;
    }

    LOG(info) << "grid starting: injected_cash=" << injected_cash << " grid_count=" << grid_count << " step_ratio=" << step_ratio << " " << g_ticket;

    g_user_data.lock();
    make_scope_exit([] { g_user_data.unlock(); });

    auto itrproduct = public_product_info_.data.find(g_ticket);
    if (itrproduct == public_product_info_.data.end()) {
        LOG(error) << "cannot fetch product " << g_ticket;
        return;
    }

    auto ccy = itrproduct->second.settle_ccy;

    auto itrbal = balance_.balval.find(ccy);
    if (itrbal == balance_.balval.end()) {
        LOG(error) << "cannot fetch balance " << ccy;
        return;
    }
    auto cash = itrbal->second;
    LOG(info) << "available cash: " << cash << " " << ccy;
    if (strtof(cash.c_str(), nullptr) < injected_cash) {
        LOG(error) << "no enough cash!";
        return;
    }

    auto itrtrades = public_trades_info_.trades_data.find(g_ticket);
    if (itrtrades == public_trades_info_.trades_data.end()) {
        LOG(error) << "cannot fetch current price.";
        return;
    }
    auto price = itrtrades->second.px;
    LOG(info) << "current price: " << price;

    float cur_price = strtof(price.c_str(), nullptr);
    if (cur_price <= 0) {
        LOG(error) << "invalid price: " << cur_price;
        return;
    }

    auto order_side = Command::OrderSide::Buy;
    auto order_pos_side = Command::OrderPosSide::Long;

    auto tick_sz = itrproduct->second.tick_sz;
    std::deque<Command::OrderData> grid_prices;
    float px = cur_price;
    float total_px = 0;

    for (int i = 0; i < grid_count; ++i) {
        px = px * (1.0f - step_ratio);
        total_px += px;

        Command::OrderData order_data;
        order_data.side = order_side;
        order_data.pos_side = order_pos_side;
        order_data.px = floatToString(px, tick_sz);
        grid_prices.push_back(order_data);
    }

    auto ct_val = strtof(itrproduct->second.ct_val.c_str(), nullptr);
    if (ct_val * total_px >= injected_cash) {
        LOG(error) << "no enmoght cash. require at least " << floatToString(ct_val * total_px, tick_sz);
        return;
    }

    auto amount = floatToString(injected_cash / total_px, itrproduct->second.ct_val);
    for (auto& v : grid_prices) {
        v.amount = amount;
    }

    auto cmd = Command::makeMultiOrderReq(g_ticket, Command::OrderType::Limit, Command::TradeMode::Isolated, grid_prices);
    g_private_channel->sendCmd(std::move(cmd),
        [this](Command::Response& resp) {
            if (resp.code == 0) {
                LOG(debug) << "<< order ok.";
            } else
                LOG(error) << "<< order failed.";
        }
    );
    

}
