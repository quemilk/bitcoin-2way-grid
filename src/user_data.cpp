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

    std::deque<OrderData> grid_orders;
    {
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

        auto tick_sz = itrproduct->second.tick_sz;

        grid_strategy_.grids.clear();
        grid_strategy_.grids.reserve(grid_count + 1);
        float px = cur_price;
        float total_px = 0;

        std::deque<std::string> d0;
        for (int i = 0; i < grid_count / 2; ++i) {
            px = px * (1.0f - step_ratio);
            total_px += px;
            d0.push_back(floatToString(px, tick_sz));
        }

        for (auto itr = d0.rbegin(); itr != d0.rend(); ++itr) {
            GridStrategy::Grid grid;
            grid.px = *itr;
            grid_strategy_.grids.push_back(grid);
        }

        px = cur_price;
        total_px += px;
        GridStrategy::Grid grid;
        auto cur_price_str = floatToString(px, tick_sz);
        grid.px = cur_price_str;
        grid_strategy_.grids.push_back(grid);

        for (int i = grid_count / 2; i < grid_count; ++i) {
            px = px * (1.0f + step_ratio);
            total_px += px;
            GridStrategy::Grid grid;
            grid.px = floatToString(px, tick_sz);
            grid_strategy_.grids.push_back(grid);
        }

        auto ct_val = strtof(itrproduct->second.ct_val.c_str(), nullptr);
        auto requred_cash = ct_val * total_px * 2;
        if (requred_cash >= injected_cash) {
            LOG(error) << "no enough cash. require at least! " << floatToString(requred_cash, tick_sz);
            return;
        }

        grid_strategy_.order_amount = floatToString(injected_cash / total_px / ct_val, itrproduct->second.lot_sz);
        if (grid_strategy_.order_amount.empty()) {
            LOG(error) << "invalid amount!";
            return;
        }

        for (size_t i = 0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];
            if (cur_price_str == grid.px)
                continue;

            GridStrategy::Grid::Order grid_order;
            grid_order.order_data.px = grid.px;
            grid_order.order_data.amount = grid_strategy_.order_amount;
            if (i < grid_strategy_.grids.size() - 1) {
                grid_order.order_data.clordid = genCliOrdId();
                grid_order.order_data.side = OrderSide::Buy;
                grid_order.order_data.pos_side = OrderPosSide::Long;
                grid_order.order_status = OrderStatus::Live;                
                grid.long_orders.push_back(grid_order);
            }
            if (i > 0) {
                grid_order.order_data.clordid = genCliOrdId();
                grid_order.order_data.side = OrderSide::Sell;
                grid_order.order_data.pos_side = OrderPosSide::Short;
                grid_order.order_status = OrderStatus::Live;
                grid.short_orders.push_back(grid_order);
            }
        }

        for (auto& grid : grid_strategy_.grids) {
            for (auto& order : grid.long_orders)
                if (!order.order_data.amount.empty())
                    grid_orders.push_back(order.order_data);
            for (auto& order : grid.short_orders)
                if (!order.order_data.amount.empty())
                    grid_orders.push_back(order.order_data);
        }
     }

     auto cmd = Command::makeMultiOrderReq(g_ticket, OrderType::Limit, TradeMode::Cross, grid_orders);
     g_private_channel->sendCmd(std::move(cmd),
         [this](Command::Response& resp) {
             if (resp.code == 0) {
                 LOG(debug) << "<< order ok.";
             } else
                 LOG(error) << "<< order failed.";
         }
     );
}

void UserData::updateGrid() {
    std::deque<OrderData> grid_orders;
    {
        g_user_data.lock();
        make_scope_exit([] { g_user_data.unlock(); });

        for (size_t i=0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];
            auto grid_next = (i < grid_strategy_.grids.size() - 1) ? &grid_strategy_.grids[i + 1] : nullptr;
            auto grid_pre = (i > 0) ? &grid_strategy_.grids[i - 1] : nullptr;

            for (auto itr = grid.long_orders.begin(); itr != grid.long_orders.end(); ) {
                auto& order_data = itr->order_data;

                if (itr->order_status == OrderStatus::Filled) {
                    itr->order_status = OrderStatus::Empty;
                    order_data.amount.clear();
                    if (order_data.side == OrderSide::Buy) {
                        if (grid_next) {
                            GridStrategy::Grid::Order new_order;
                            new_order.order_data.clordid = genCliOrdId();
                            new_order.order_data.px = grid_next->px;
                            new_order.order_data.amount = grid_strategy_.order_amount;
                            new_order.order_data.side = OrderSide::Sell;
                            new_order.order_data.pos_side = OrderPosSide::Long;
                            new_order.order_status = OrderStatus::Live;
                            grid_next->long_orders.push_back(new_order);
                            grid_orders.push_back(new_order.order_data);
                        }
                    } else if (order_data.side == OrderSide::Sell) {
                        if (grid_pre) {
                            GridStrategy::Grid::Order new_order;
                            new_order.order_data.clordid = genCliOrdId();
                            new_order.order_data.px = grid_pre->px;
                            new_order.order_data.amount = grid_strategy_.order_amount;
                            new_order.order_data.side = OrderSide::Buy;
                            new_order.order_data.pos_side = OrderPosSide::Long;
                            new_order.order_status = OrderStatus::Live;
                            grid_pre->long_orders.push_back(new_order);
                            grid_orders.push_back(new_order.order_data);
                        }
                    }
                }
                
                if (itr->order_status == OrderStatus::Canceled || itr->order_status == OrderStatus::Empty) {
                    itr = grid.long_orders.erase(itr);
                } else
                    ++itr;
            }

            for (auto itr = grid.short_orders.begin(); itr != grid.short_orders.end(); ) {
                auto& order_data = itr->order_data;
                if (itr->order_status != OrderStatus::Canceled) {
                    itr = grid.short_orders.erase(itr);
                    continue;
                }

                if (itr->order_status == OrderStatus::Filled) {
                    itr->order_status = OrderStatus::Empty;
                    order_data.amount.clear();
                    if (order_data.side == OrderSide::Buy) {
                        if (grid_next) {
                            GridStrategy::Grid::Order new_order;
                            new_order.order_data.clordid = genCliOrdId();
                            new_order.order_data.px = grid_next->px;
                            new_order.order_data.amount = grid_strategy_.order_amount;
                            new_order.order_data.side = OrderSide::Sell;
                            new_order.order_data.pos_side = OrderPosSide::Short;
                            new_order.order_status = OrderStatus::Live;
                            grid_next->short_orders.push_back(new_order);
                            grid_orders.push_back(new_order.order_data);
                        }
                    } else if (order_data.side == OrderSide::Sell) {
                        if (grid_pre) {
                            GridStrategy::Grid::Order new_order;
                            new_order.order_data.clordid = genCliOrdId();
                            new_order.order_data.px = grid_pre->px;
                            new_order.order_data.amount = grid_strategy_.order_amount;
                            new_order.order_data.side = OrderSide::Buy;
                            new_order.order_data.pos_side = OrderPosSide::Short;
                            new_order.order_status = OrderStatus::Live;
                            grid_pre->short_orders.push_back(new_order);
                            grid_orders.push_back(new_order.order_data);
                        }
                    }
                }

                if (itr->order_status == OrderStatus::Canceled || itr->order_status == OrderStatus::Empty) {
                    itr = grid.short_orders.erase(itr);
                } else
                    ++itr;
            }
        }
    }

    if (!grid_orders.empty()) {
        auto cmd = Command::makeMultiOrderReq(g_ticket, OrderType::Limit, TradeMode::Cross, grid_orders);
        g_private_channel->sendCmd(std::move(cmd),
            [this](Command::Response& resp) {
                if (resp.code == 0) {
                    LOG(debug) << "<< order ok.";
                } else
                    LOG(error) << "<< order failed.";
            }
        );
    }
}

