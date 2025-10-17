#pragma once

#include "order.hpp"
#include "trade.hpp"
#include <queue>
#include <nlohmann/json.hpp>
#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <functional>




class OrderBook{
    private:
        std::string symbol;
        std::priority_queue<Order,std::vector<Order>,std::function<bool(const Order&,const Order&)>> bids;
        std::priority_queue<Order,std::vector<Order>,std::function<bool(const Order&,const Order&)>> asks;
        std::map<double,double, std::greater<double>> bid_levels; // Price -> Quantity
        std::map<double,double,std::less<double>> ask_levels; // Price -> Quantity
        std::vector<Trade> trades;
        std::mutex mtx;
        static std::atomic<uint64_t> order_id_counter;
        double maker_fee_rate = 0.001; 
        double taker_fee_rate = 0.002; 
        


    public:
        OrderBook(const std::string& symbol);
        std::vector<Trade> addOrder(double price, double quantity, std::string side, std::string type);
        std::vector<Trade> limitOrder(Order& order);
        std::vector<Trade> marketOrder(Order& order);
        std::vector<Trade> iocOrder(Order& order);
        std::vector<Trade> fokOrder(Order& order);
        void updateLevel(Side side,double price,double quantity);
        std::pair<std::pair<double, double>, std::pair<double, double>> getBBO() const;
        nlohmann::json getSnapshot(size_t depth) const;
        std::function<void (const Trade&)> trade_callback;
        double calculateFee(Side side, double amount) const;
        
};
