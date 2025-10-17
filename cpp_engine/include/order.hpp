#pragma once

#include <string>
#include <chrono>

enum class Side { BUY,SELL};
enum class OrderType { LIMIT, MARKET,IOC,FOK};
using Price  = double;
using Qty    = double;
using Timestamp = uint64_t;

struct Order {
    uint64_t order_id;
    Side side;
    OrderType type;
    Price price;
    Qty quantity;
    Timestamp timestamp;

    Order(uint64_t id, Side s, OrderType t, Price p, Qty q)
        : order_id(id), side(s), type(t), price(p), quantity(q), timestamp(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count()) {}
};

