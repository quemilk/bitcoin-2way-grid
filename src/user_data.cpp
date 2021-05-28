﻿#include "user_data.h"
#include "private_channel.h"
#include "logger.h"
#include "command.h"
#include <deque>

UserData g_user_data;
extern std::string g_ticket;


static std::string floatToString(float f, const std::string& tick_sz) {
    std::vector<std::string> v;
    splitString(tick_sz, v, '.');

    if (v.size() == 1) {
        auto l = strtol(v[0].c_str(), nullptr, 0);
        if (l)
            return std::to_string((int)(f / l * l));
    } else if (v.size() == 2) {
        auto absf = abs(f);
        auto l = strtol(v[0].c_str(), nullptr, 0);
        auto d = strtol(v[1].c_str(), nullptr, 0);
        int power = 1;
        int digit = v[1].size();
        for (int i = 0; i < digit; ++i)
            power *= 10;
        int r = (int)(absf * power / (l * power + d));
        auto ipart = std::to_string((int)(r / power));
        auto fpart = std::to_string((int)(r % power));
        std::string o = (f < 0 ? "-" : "") + ipart;
        if (digit > 0) {
            o += ".";
            for (int i = 0; i < digit - (int)fpart.size(); ++i)
                o += "0";
            o += fpart;
        }
        return o;
    } else {
        return std::to_string(f);
    }
    return std::string();
}

static std::string calcDiff(const std::string& px1, const std::string& px2, const std::string& tick_sz, const string& amount) {
    if (px1.empty() || px2.empty())
        return std::string();
    float a = strtof(amount.c_str(), nullptr);
    float f1 = strtof(px1.c_str(), nullptr) * a;
    float f2 = strtof(px2.c_str(), nullptr) * a;
    return (f1 > f2 ? "+" : "") + floatToString(f1 - f2, tick_sz);
}


void UserData::startGrid(GridStrategy::Option option) {
    option.grid_count &= ~1;
    if (option.grid_count <= 0 || option.step_ratio <= 0 || option.step_ratio >= 1.0f) {
        LOG(error) << "invalid param!";
        return;
    }

    LOG(info) << "grid starting: injected_cash=" << option.injected_cash
        << " grid_count=" << option.grid_count << " step_ratio=" << option.step_ratio << " " << g_ticket;

    std::deque<OrderData> grid_orders;
    {
        g_user_data.lock();
        auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

        if (!grid_strategy_.grids.empty()) {
            LOG(error) << "grid already running";
            return;
        }

        auto failed_sp = make_scope_exit([&] {
            grid_strategy_.grids.clear(); 
            }
        );

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

        grid_strategy_.tick_sz = itrproduct->second.tick_sz;
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
        auto lot_sz = itrproduct->second.lot_sz;
        float lot_sz_v = strtof(lot_sz.c_str(), nullptr);
        float min_sz_v = strtof(itrproduct->second.min_sz.c_str(), nullptr);

        grid_strategy_.grids.clear();
        grid_strategy_.grids.reserve(option.grid_count + 1);
        float px = cur_price;

        std::deque<std::string> d0;
        float total_sum = 0;

        for (int i = 0; i < option.grid_count / 2; ++i) {
            px = px * (1.0f - option.step_ratio);
            auto px_str = floatToString(px, tick_sz);
            auto fixed_px = strtof(px_str.c_str(), nullptr);
            total_sum += lot_sz_v * fixed_px;
            d0.push_back(px_str);
        }

        for (auto itr = d0.rbegin(); itr != d0.rend(); ++itr) {
            GridStrategy::Grid grid;
            grid.px = *itr;
            grid_strategy_.grids.push_back(grid);
        }

        px = cur_price;
        total_sum += lot_sz_v * px;
        GridStrategy::Grid grid;
        auto cur_price_str = floatToString(px, tick_sz);
        grid.px = cur_price_str;
        grid_strategy_.grids.push_back(grid);

        for (int i = option.grid_count / 2; i < option.grid_count; ++i) {
            px = px * (1.0f + option.step_ratio);
            auto px_str = floatToString(px, tick_sz);
            auto fixed_px = strtof(px_str.c_str(), nullptr);
            total_sum += lot_sz_v * fixed_px;
            GridStrategy::Grid grid;
            grid.px = px_str;
            grid_strategy_.grids.push_back(grid);
        }


        auto ct_val = strtof(itrproduct->second.ct_val.c_str(), nullptr);
        auto requred_cash = ct_val * total_sum * 2;
        auto amount = floatToString(option.injected_cash / requred_cash, lot_sz);
        if (requred_cash >= option.injected_cash || strtof(amount.c_str(), nullptr) < min_sz_v) {
            LOG(error) << "no enough cash. require at least! " << floatToString(requred_cash, tick_sz);
            return;
        }

        for (int i = 0; i < option.grid_count; ++i) {
            grid_strategy_.grids[i].order_amount = amount;
        }

        for (size_t i = 0; i < grid_strategy_.grids.size(); ++i) {
            auto& grid = grid_strategy_.grids[i];

            GridStrategy::Grid::Order grid_order;
            grid_order.order_data.px = grid.px;
            grid_order.order_data.amount = grid.order_amount;
            if (i <= grid_strategy_.grids.size() / 2) {
                grid_order.order_data.clordid = genCliOrdId();
                grid_order.order_data.side = OrderSide::Buy;
                grid_order.order_data.pos_side = OrderPosSide::Long;
                grid_order.order_status = OrderStatus::Live;
                grid.long_orders.orders.push_back(grid_order);
                grid.long_orders.init_ordered = true;

                auto new_order = grid_order.order_data;
                if (i == grid_strategy_.grids.size() / 2)
                    new_order.order_type = OrderType::Market;
                grid_orders.push_back(new_order);
            }
            if (i >= grid_strategy_.grids.size() / 2) {
                grid_order.order_data.clordid = genCliOrdId();
                grid_order.order_data.side = OrderSide::Sell;
                grid_order.order_data.pos_side = OrderPosSide::Short;
                grid_order.order_status = OrderStatus::Live;
                grid.short_orders.orders.push_back(grid_order);
                grid.short_orders.init_ordered = true;

                auto new_order = grid_order.order_data;
                if (i == grid_strategy_.grids.size() / 2)
                    new_order.order_type = OrderType::Market;
                grid_orders.push_back(new_order);
            }
        }

        failed_sp.cancel();
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
        auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

        if (grid_strategy_.grids.empty()) {
            sell_count = buy_count = -1;
            return;
        }

        for (size_t igrid=0; igrid < grid_strategy_.grids.size(); ++igrid) {
            auto& grid = grid_strategy_.grids[igrid];
            auto grid_next = (igrid < grid_strategy_.grids.size() - 1) ? &grid_strategy_.grids[igrid + 1] : nullptr;
            auto grid_pre = (igrid > 0) ? &grid_strategy_.grids[igrid - 1] : nullptr;

            GridStrategy::Grid::OrdersQueue* orders_arr[] = { &grid.long_orders, &grid.short_orders };
            GridStrategy::Grid::OrdersQueue* next_orders_arr[] = { grid_next ? &grid_next->long_orders : nullptr,  grid_next ? &grid_next->short_orders : nullptr };
            GridStrategy::Grid::OrdersQueue* pre_orders_arr[] = { grid_pre ? &grid_pre->long_orders : nullptr,  grid_pre ? &grid_pre->short_orders : nullptr };
            
            OrderPosSide pos_sides[] = { OrderPosSide::Long, OrderPosSide::Short };

            for (int i = 0; i < 2; ++i) {
                auto& orders = orders_arr[i]->orders;
                auto pos_side = pos_sides[i];
                bool has_filled = false;

                for (auto itr = orders.begin(); itr != orders.end(); ) {
                    auto& order_data = itr->order_data;

                    if (itr->order_status == OrderStatus::Filled) {
                        has_filled = true;
                        itr->order_status = OrderStatus::Empty;
                        if (order_data.side == OrderSide::Buy) {
                            if (grid_next && next_orders_arr[i]) {
                                GridStrategy::Grid::Order new_order;
                                new_order.order_data.clordid = genCliOrdId();
                                new_order.order_data.px = grid_next->px;
                                new_order.order_data.amount = order_data.amount;
                                new_order.order_data.side = OrderSide::Sell;
                                new_order.order_data.pos_side = pos_side;
                                new_order.order_status = OrderStatus::Live;
                                if (pos_side == OrderPosSide::Long)
                                    new_order.fill_px = itr->fill_px;
                                next_orders_arr[i]->orders.push_back(new_order);
                                grid_orders.push_back(new_order.order_data);
                            }
                        } else if (order_data.side == OrderSide::Sell) {
                            if (grid_pre && pre_orders_arr[i]) {
                                GridStrategy::Grid::Order new_order;
                                new_order.order_data.clordid = genCliOrdId();
                                new_order.order_data.px = grid_pre->px;
                                new_order.order_data.amount = order_data.amount;
                                new_order.order_data.side = OrderSide::Buy;
                                new_order.order_data.pos_side = pos_side;
                                new_order.order_status = OrderStatus::Live;
                                if (pos_side == OrderPosSide::Short)
                                    new_order.fill_px = itr->fill_px;
                                pre_orders_arr[i]->orders.push_back(new_order);
                                grid_orders.push_back(new_order.order_data);
                            }
                        }
                    }

                    if (itr->order_status == OrderStatus::Canceled || itr->order_status == OrderStatus::Empty) {
                        itr = orders.erase(itr);
                    } else
                        ++itr;
                }

                if (has_filled && !orders_arr[i]->init_ordered) {
                    orders_arr[i]->init_ordered = true;
                    GridStrategy::Grid::Order new_order;
                    new_order.order_data.clordid = genCliOrdId();
                    new_order.order_data.px = grid.px;
                    new_order.order_data.amount = grid.order_amount;
                    if (pos_side == OrderPosSide::Long)
                        new_order.order_data.side = OrderSide::Buy;
                    else if (pos_side == OrderPosSide::Short)
                        new_order.order_data.side = OrderSide::Sell;
                    new_order.order_data.pos_side = pos_side;
                    new_order.order_status = OrderStatus::Live;
                    orders_arr[i]->orders.push_back(new_order);
                    grid_orders.push_back(new_order.order_data);
                }
            }
        }

        for (size_t igrid = 0; igrid < grid_strategy_.grids.size(); ++igrid) {
            auto& grid = grid_strategy_.grids[igrid];
            GridStrategy::Grid::OrdersQueue* orders_arr[] = { &grid.long_orders, &grid.short_orders };

            for (int i = 0; i < 2; ++i) {
                auto& orders = *orders_arr[i];
                for (auto& order : orders.orders) {
                    if (order.order_data.side == OrderSide::Buy)
                        ++buy_count;
                    else if (order.order_data.side == OrderSide::Sell)
                        ++sell_count;
                }
            }
        }
    }

    if (buy_count == 0 || sell_count == 0) {
        LOG(warning) << "WARNING!!! exeed to grid limit. closeout the grid.";
        clearGrid();
        std::this_thread::sleep_for(std::chrono::seconds(5));
        LOG(warning) << "WARNING!!! grid will restart on 5min.";
        std::thread(
            [] {
                std::this_thread::sleep_for(std::chrono::minutes(5));
                g_user_data.startGrid(g_user_data.grid_strategy_.option);
            }
        ).detach();
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
        auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

        for (auto& grid : grid_strategy_.grids) {
            auto orders_arr = { &grid.long_orders, &grid.short_orders };
            for (auto ordersq : orders_arr) {
                for (auto& order : ordersq->orders) {
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
                } else if (resp.code == 1 || resp.code == 2) {
                    LOG(debug) << "<< cancel partical ok.";
                } else
                    LOG(error) << "<< cancel failed. " << resp.data;
            }
        );

    }
}

std::string UserData::currentPrice() {
    g_user_data.lock();
    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

    auto itrtrades = public_trades_info_.trades_data.find(g_ticket);
    if (itrtrades != public_trades_info_.trades_data.end())
        return itrtrades->second.px;
    return std::string();
}

std::string UserData::currentCash(std::string ccy) {
    g_user_data.lock();
    auto scoped_exit = make_scope_exit([] { g_user_data.unlock(); });

    auto itrbal = g_user_data.balance_.balval.find(grid_strategy_.ccy);
    if (itrbal != g_user_data.balance_.balval.end()) {
        return itrbal->second;
    }
    return std::string();
}

std::ostream& operator << (std::ostream& o, const UserData::GridStrategy& t) {
    o << "=====Grid=====" << std::endl;
    o << g_ticket;

    auto current_cash = strtof(g_user_data.currentCash(t.ccy).c_str(), nullptr);
    if (current_cash) {
        if (current_cash >= t.start_cash) {
            o << " +" << current_cash - t.start_cash;
        } else {
            o << " -" << t.start_cash - current_cash;
        }
        o << " " << t.ccy;

        if (t.start_cash != t.origin_cash) {
            o << " " << t.ccy << " \ttotal: ";
            if (current_cash >= t.origin_cash) {
                o << " +" << current_cash - t.origin_cash;
            } else {
                o << " -" << t.origin_cash - current_cash;
            }
            o << " " << t.ccy;
        }
    }
    o << std::endl;
    o << "inject cash: " << t.option.injected_cash
        << ", grid count: " << t.option.grid_count << ", grid step: " << t.option.step_ratio
        << ", origin: " << t.origin_cash << std::endl;

    auto cur_px_str = g_user_data.currentPrice();
    if (!cur_px_str.empty())
        o << "current: " << cur_px_str << std::endl;

    o << "  long:" << std::endl;
    for (auto itr = t.grids.rbegin(); itr != t.grids.rend(); ++itr) {
        auto& v = *itr;
        o << "    * " << v.px;

        if (!v.long_orders.orders.empty()) {
            for (auto& order : v.long_orders.orders) {
                auto long_side = order.order_data.amount.empty() ? "  " : toString(order.order_data.side);
                o << " \t" << long_side << " \t" << order.order_data.amount << " \t" << calcDiff(cur_px_str, order.fill_px, t.tick_sz, order.order_data.amount);
            }
        }
        o << std::endl;
    }

    o << "  short:" << std::endl;
    for (auto itr = t.grids.rbegin(); itr != t.grids.rend(); ++itr) {
        auto& v = *itr;
        o << "    * " << v.px;

        if (!v.short_orders.orders.empty()) {
            for (auto& order : v.short_orders.orders) {
                auto long_side = order.order_data.amount.empty() ? "  " : toString(order.order_data.side);
                o << " \t" << long_side << " \t" << order.order_data.amount << " \t" << calcDiff(order.fill_px, cur_px_str, t.tick_sz, order.order_data.amount);
            }
        }
        o << std::endl;
    }
    return o;
}
