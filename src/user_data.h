#pragma once

#include "type.h"
#include "util.h"
#include <mutex>
#include <ostream>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <iomanip>
#include <chrono>

class UserData {
public:
    struct Balance {
        struct BalVal {
            std::string eq;
            std::string cash_bal;
            std::string upl;
            std::string avail_eq;
        };
        std::map<std::string, BalVal> balval;
        bool inited = false;

        friend class std::ostream& operator << (std::ostream& o, const Balance& t) {
            o << "=====Balance=====" << std::endl;
            for (auto& v : t.balval) {
                o << "  " << v.first << " \teq: " << v.second.eq << ", cash: " << v.second.cash_bal << ", upl: " << v.second.upl << std::endl;
            }
            return o;
        }
    };


    struct Position {
        struct PosData {
            std::string pos_id;
            std::string inst_id;
            std::string inst_type;
            std::string pos_side;
            std::string avg_px;
            std::string pos;
            std::string ccy;
            uint64_t utime_msec;
        };

        std::map<std::string /*pos_id*/, PosData> posval;

        friend class std::ostream& operator << (std::ostream& o, const Position& t) {
            o << "=====Position=====" << std::endl;

            for (auto& pos : t.posval) {
                auto& v = pos.second;
                o << "  - pos: " << pos.first << " " << pos.second.inst_id << "  " << pos.second.inst_type << std::endl;
                o << "    " << v.pos_side << " \t" << v.pos << " \t" << v.avg_px << " \t" << v.ccy << "\t" << toTimeStr(v.utime_msec) << std::endl;
            }
            return o;
        }
    };

    struct PublicTradesInfo {
        struct Info {
            std::string inst_id;
            std::string trade_id;
            std::string px;
            std::string sz;
            std::string pos_side;
            uint64_t ts;
        };
        std::map<std::string, Info> trades_data;
    };

    struct ProductInfo {
        struct Info {
            std::string inst_id;
            std::string inst_type;
            std::string lot_sz;
            std::string min_sz;
            std::string tick_sz;
            std::string settle_ccy;
            std::string ct_val;
            std::string ct_multi;
        };
        std::map<std::string, Info> data;

        friend class std::ostream& operator << (std::ostream& o, const ProductInfo& t) {
            o << "=====Instruments=====" << std::endl;

            for (auto& product : t.data) {
                auto& v = product.second;
                o << "  - " << product.first << " " << v.inst_type << " " << v.settle_ccy << std::endl;
                o << "    lot_sz:" << v.lot_sz << " min_sz:" << v.min_sz << " tick_siz:" << v.tick_sz << " ct_val:" << v.ct_val << std::endl;
            }
            return o;
        }
    };


    struct GridStrategy {
        struct Grid {
            std::string px;
            struct Order {
                OrderData order_data;
                OrderStatus order_status = OrderStatus::Empty;
                std::string fill_px;
            };

            struct OrdersQueue {
                bool init_ordered = false;
                std::deque<Order> orders;
                std::string order_amount;
            };
            OrdersQueue long_orders, short_orders;
        };

        struct Option {
            float injected_cash = 0;
            int grid_count = 10;
            float step_ratio = 0.01f;
            int leverage = 10;
        };

        Option option;

        std::vector<Grid> grids;

        std::string ccy;
        std::string tick_sz;
        std::string ct_val;
        float origin_cash = 0;
        float start_cash = 0;
        std::chrono::steady_clock::time_point start_time;
        int rebuild = 0;

        friend std::ostream& operator << (std::ostream& o, const GridStrategy& t);
    };

public:
    void startGrid(GridStrategy::Option option, bool conetinue_last_grid=false, bool is_test=false);
    void updateGrid();
    void clearGrid();

    std::string currentPrice();
    std::string currentCash(std::string ccy);

public:
    void lock() {
        mutex_.lock();
    }

    void unlock() {
        mutex_.unlock();
    }

public:
    Balance balance_;
    Position position_;
    PublicTradesInfo public_trades_info_;
    ProductInfo public_product_info_;
    GridStrategy grid_strategy_;

private:
    std::recursive_mutex mutex_;
};

extern UserData g_user_data;
