#include "user_data.h"
#include "logger.h"
#include <deque>

extern std::string g_ticket;

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
            for (int i = 0; i < digit - (int)fpart.size(); ++i)
                o += "0";
            o += "." + fpart;
        }
        return o;
    } else {
        assert(false);
    }
    return std::string();
}


void UserData::startGrid(int count, float step_ratio) {
    if (count <= 0 || step_ratio <= 0 || step_ratio >= 1.0f) {
        LOG(error) << "invalid param!";
        return;
    }

    LOG(info) << "grid starting: count=" << count << " step_ratio=" << step_ratio << " " << g_ticket;

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
    LOG(info) << "cash: " << cash << " " << ccy;


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

    auto tick_sz = itrproduct->second.tick_sz;
    std::deque<std::pair<std::string, std::string> > grid_prices;
    float px = cur_price;
    for (int i = 0; i < count; ++i) {
        px = px * (1.0f - step_ratio);
        grid_prices.push_back({ floatToString(px, tick_sz), "" });
    }


}
