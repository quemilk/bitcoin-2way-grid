#include "user_data.h"
#include "logger.h"

extern std::string g_ticket;

UserData g_user_data;

void UserData::startGrid(int count, float step_ratio) {
    LOG(info) << "grid starting: count=" << count << " step_ratio=" << step_ratio;

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



}
