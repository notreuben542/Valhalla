#include "include/order_book.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <random>

void prefillOrderBook(OrderBook &ob, int levels, int qty_per_level) {
    // Fill BIDs
    for (int i = 0; i < levels; ++i) {
        double price = 60000 - i; // descending for bids
        ob.addOrder(price, qty_per_level, "BUY", "LIMIT");
    }

    // Fill ASKS
    for (int i = 0; i < levels; ++i) {
        double price = 60001 + i; // ascending for asks
        ob.addOrder(price, qty_per_level, "SELL", "LIMIT");
    }
}

void benchmarkOrders(OrderBook &ob, int num_orders, bool market) {
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> price_dist(59950, 60050);
    std::uniform_real_distribution<double> qty_dist(0.01, 2.0);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_orders; ++i) {
        double price = price_dist(gen);
        double qty = qty_dist(gen);
        std::string side = (i % 2 == 0) ? "BUY" : "SELL";
        std::string type = market ? "MARKET" : "LIMIT";

        ob.addOrder(price, qty, side, type);
    }

    auto end = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration<double>(end - start).count();
    std::cout << (market ? "MARKET" : "LIMIT")
              << " orders processed: " << num_orders
              << " in " << duration_sec << " sec ("
              << num_orders / duration_sec << " orders/sec)\n";
}

int main() {
    OrderBook ob("BTC-USDT");

    // Step 1: Pre-fill book with 200,000 levels each side
    std::cout << "Prefilling order book...\n";
    prefillOrderBook(ob, 200000, 1);

    // Step 2: Benchmark MARKET orders
    benchmarkOrders(ob, 200000, true);

    // Step 3: Benchmark LIMIT orders
    benchmarkOrders(ob, 200000, false);

    return 0;
}
