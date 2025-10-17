#pragma once

#include <string>
#include "order.hpp"

struct Trade {
    uint64_t trade_id;
    std::string symbol;
    Price price;
    Qty quantity;
    Timestamp timestamp;
    uint64_t maker_order_id;
    uint64_t taker_order_id;
    Side aggressor_side;
    double maker_fee = 0.0;
    double taker_fee = 0.0;
};


