#pragma once

#include "type.h"
#include "util.h"
#include <mutex>
#include <map>
#include <set>
#include <ostream>

class UserData {
public:
    struct Balance {
        std::map<std::string, std::string> balval;

        friend class std::ostream& operator << (std::ostream& o, const Balance& t) {
            o << "=====Balance=====" << std::endl;
            for (auto& v : t.balval) {
                o << "  " << v.first << " \tcash: " << v.second << std::endl;
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
            std::string lot_sz;
            std::string min_sz;
            std::string tick_sz;
            std::string settle_ccy;
            std::string ct_val;
            std::string ct_multi;
        };
        std::map<std::string, Info> data;
    };


    struct StrategyOrders {
        struct OrderStatus {
            OrderData order_data;
        };

        std::map<std::string /* cliordid */, OrderStatus> orders;
    };

public:
    void startGrid(float injected_cash, int grid_count, float step_ratio);

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

private:
    std::mutex mutex_;
};

extern UserData g_user_data;
