#include "user_data.h"
#include "private_channel.h"
#include "logger.h"
#include "command.h"
#include <deque>


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


void UserData::startGrid(GridStrategy::Option option) {
    if (option.grid_count <= 0 || option.step_ratio <= 0 || option.step_ratio >= 1.0f) {
        LOG(error) << "invalid param!";
        return;
    }

    LOG(info) << "grid starting: injected_cash=" << option.injected_cash
        << " grid_count=" << option.grid_count << " step_ratio=" << option.step_ratio << " " << g_ticket;

    std::deque<OrderData> grid_orders;
    {
        g_user_data.lock();
        make_scope_exit([] { g_user_data.unlock(); });

        if (!grid_strategy_.grids.empty()) {
            LOG(error, "grid already running");
            return;
        }
        grid_strategy_.option = option;

        auto itrproduct = public_product_info_.data.find(g_ticket);
        if (itrproduct == public_product_info_.data.end()) {
            LOG(error) << "cannot fetch product " << g_ticket;
            return;
        }

        if (itrproduct->second.inst_type != "SWAP") {
            LOG(error) << "only support SWAP type!";
            return;
        }

        auto ccy = itrproduct->second.settle_ccy;

        auto itrbal = balance_.balval.find(ccy);
        if (itrbal == balance_.balval.end()) {
            LOG(error) << "cannot fetch balance " << ccy;
            return;
        }
        auto cash = itrbal->second;
        auto cashval = strtof(cash.c_str(), nullptr);
        LOG(info) << "available cash: " << cash << " " << ccy;
        if (cashval < option.injected_cash) {
            LOG(error) << "no enough cash!";
            return;
        }
        grid_strategy_.ccy = ccy;
        if (0 == grid_strategy_.origin_cash)
            grid_strategy_.origin_cash = cashval;
        grid_strategy_.start_cash = cashval;
        grid_strategy_.current_cash = 0;

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
        grid_strategy_.grids.reserve(option.grid_count + 1);
        float px = cur_price;
        float total_px = 0;

        std::deque<std::string> d0;
        for (int i = 0; i < option.grid_count / 2; ++i) {
            px = px * (1.0f - option.step_ratio);
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

        for (int i = option.grid_count / 2; i < option.grid_count; ++i) {
            px = px * (1.0f + option.step_ratio);
            total_px += px;
            GridStrategy::Grid grid;
            grid.px = floatToString(px, tick_sz);
            grid_strategy_.grids.push_back(grid);
        }

        auto ct_val = strtof(itrproduct->second.ct_val.c_str(), nullptr);
        auto requred_cash = ct_val * total_px * 2;
        if (requred_cash >= option.injected_cash) {
            LOG(error) << "no enough cash. require at least! " << floatToString(requred_cash, tick_sz);
            return;
        }

        grid_strategy_.order_amount = floatToString(option.injected_cash / total_px / ct_val, itrproduct->second.lot_sz);
        if (grid_strategy_.order_amount.empty()) {
            LOG(error) << "invalid amount!";
            return;
        }

        for (size_t i = 0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];

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

        for (size_t i = 0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];
            for (auto& order : grid.long_orders) {
                if (!order.order_data.amount.empty()) {
                    auto new_order = order.order_data;
                    if (i >= grid_strategy_.grids.size() / 2)
                        new_order.order_type = OrderType::Market;
                    grid_orders.push_back(new_order);
                }
            }
            for (auto& order : grid.short_orders) {
                if (!order.order_data.amount.empty()) {
                    auto new_order = order.order_data;
                    if (i <= grid_strategy_.grids.size() / 2)
                        new_order.order_type = OrderType::Market;
                    grid_orders.push_back(new_order);
                }
            }
        }
     }

    while (!grid_orders.empty()) {
        auto cmd = Command::makeMultiOrderReq(g_ticket, TradeMode::Cross, grid_orders);
        g_private_channel->sendCmd(std::move(cmd),
            [this](Command::Response& resp) {
                if (resp.code == 0) {
                    LOG(debug) << "<< order ok.";
                } else
                    LOG(error) << "<< order failed. " << resp.data;
            }
        );
    }
}

void UserData::updateGrid() {
    std::deque<OrderData> grid_orders;
    int sell_count = 0, buy_count = 0;
    {
        g_user_data.lock();
        make_scope_exit([] { g_user_data.unlock(); });

        for (size_t i=0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];
            auto grid_next = (i < grid_strategy_.grids.size() - 1) ? &grid_strategy_.grids[i + 1] : nullptr;
            auto grid_pre = (i > 0) ? &grid_strategy_.grids[i - 1] : nullptr;

            std::deque<GridStrategy::Grid::Order>* orders_arr[] = { &grid.long_orders, &grid.short_orders };
            std::deque<GridStrategy::Grid::Order>* next_orders_arr[] = { grid_next ? &grid_next->long_orders : nullptr,  grid_next ? &grid_next->short_orders : nullptr };
            std::deque<GridStrategy::Grid::Order>* pre_orders_arr[] = { grid_pre ? &grid_pre->long_orders : nullptr,  grid_pre ? &grid_pre->short_orders : nullptr };
            
            OrderPosSide pos_sides[] = { OrderPosSide::Long, OrderPosSide::Short };

            for (int i = 0; i < 2; ++i) {
                auto& orders = *orders_arr[i];
                auto pos_side = pos_sides[i];

                for (auto itr = orders.begin(); itr != orders.end(); ) {
                    auto& order_data = itr->order_data;
                    if (order_data.side == OrderSide::Buy)
                        ++buy_count;
                    else if (order_data.side == OrderSide::Sell)
                        ++sell_count;

                    if (itr->order_status == OrderStatus::Filled) {
                        itr->order_status = OrderStatus::Empty;
                        order_data.amount.clear();
                        if (order_data.side == OrderSide::Buy) {
                            if (grid_next && next_orders_arr[i]) {
                                GridStrategy::Grid::Order new_order;
                                new_order.order_data.clordid = genCliOrdId();
                                new_order.order_data.px = grid_next->px;
                                new_order.order_data.amount = grid_strategy_.order_amount;
                                new_order.order_data.side = OrderSide::Sell;
                                new_order.order_data.pos_side = pos_side;
                                new_order.order_status = OrderStatus::Live;
                                next_orders_arr[i]->push_back(new_order);
                                grid_orders.push_back(new_order.order_data);
                            }
                        } else if (order_data.side == OrderSide::Sell) {
                            if (grid_pre && pre_orders_arr[i]) {
                                GridStrategy::Grid::Order new_order;
                                new_order.order_data.clordid = genCliOrdId();
                                new_order.order_data.px = grid_pre->px;
                                new_order.order_data.amount = grid_strategy_.order_amount;
                                new_order.order_data.side = OrderSide::Buy;
                                new_order.order_data.pos_side = pos_side;
                                new_order.order_status = OrderStatus::Live;
                                pre_orders_arr[i]->push_back(new_order);
                                grid_orders.push_back(new_order.order_data);
                            }
                        }
                    }

                    if (itr->order_status == OrderStatus::Canceled || itr->order_status == OrderStatus::Empty) {
                        itr = orders.erase(itr);
                    } else
                        ++itr;
                }
            }
        }

        auto itrbal = balance_.balval.find(grid_strategy_.ccy);
        if (itrbal != balance_.balval.end()) {
            grid_strategy_.current_cash = strtof(itrbal->second.c_str(), nullptr);
        }
    }

    if (buy_count == 0 && sell_count == 0) {
        LOG(warning) << "WARNING!!! exeed to grid limit. closeout the grid.";
        clearGrid();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LOG(warning) << "WARNING!!! grid will restart on 5min.";
        std::thread thread(
            [] {
                std::this_thread::sleep_for(std::chrono::minutes(5));
                g_user_data.startGrid(g_user_data.grid_strategy_.option);
            }
        );
        return;
    }

    while (!grid_orders.empty()) {
        auto cmd = Command::makeMultiOrderReq(g_ticket, TradeMode::Cross, grid_orders);
        g_private_channel->sendCmd(std::move(cmd),
            [this](Command::Response& resp) {
                if (resp.code == 0) {
                    LOG(debug) << "<< order ok.";
                } else
                    LOG(error) << "<< order failed. " << resp.data;
            }
        );
    }
}

void UserData::clearGrid() {
    std::deque<std::string> to_cancel_cliordids;
    std::deque<OrderData> to_clear_orders;
    {
        g_user_data.lock();
        make_scope_exit([] { g_user_data.unlock(); });

        for (auto& grid : grid_strategy_.grids) {
            auto orders_arr = { &grid.long_orders, &grid.short_orders };
            for (auto orders : orders_arr) {
                for (auto& order : *orders) {
                    if (!order.order_data.amount.empty()) {
                        if (order.order_status == OrderStatus::Live) {
                            to_cancel_cliordids.push_back(order.order_data.clordid);

                            bool to_clear = false;
                            if (order.order_data.pos_side == OrderPosSide::Long) {
                                if (order.order_data.side == OrderSide::Sell)
                                    to_clear = true;
                            } else if (order.order_data.pos_side == OrderPosSide::Short) {
                                if (order.order_data.side == OrderSide::Buy)
                                    to_clear = true;
                            }

                            if (to_clear) {
                                auto new_order = order.order_data;
                                new_order.clordid = genCliOrdId();
                                new_order.order_type = OrderType::Market;
                                to_clear_orders.push_back(new_order);
                            }
                        }
                    }
                }
            }
        }
        grid_strategy_.grids.clear();
    }

    struct DoneTask {
        ~DoneTask() {
            if (!r.data.empty()) {
                g_private_channel->sendCmd(std::move(r),
                    [](Command::Response& resp) {
                        if (resp.code == 0) {
                            LOG(debug) << "<< order ok.";
                        } else
                            LOG(error) << "<< order failed." << resp.data;
                    }
                );
            }
        }
        Command::Request r;
    };

    auto t = std::make_shared<DoneTask>();
    if (!to_clear_orders.empty())
        t->r = Command::makeMultiOrderReq(g_ticket, TradeMode::Cross, to_clear_orders);

    while (!to_cancel_cliordids.empty()) {
        auto cmd = Command::makeCancelMultiOrderReq(g_ticket, to_cancel_cliordids);
        g_private_channel->sendCmd(std::move(cmd),
            [t] (Command::Response& resp) {
                if (resp.code == 0) {
                    LOG(debug) << "<< cancel ok.";
                } else
                    LOG(error) << "<< cancel failed. " << resp.data;
            }
        );

    }


}
