#pragma once

#include "order.hpp"
#include "trade.hpp"
#include <queue>
#include <vector>
#include <string>
#include <mutex>
#include <functional>




class OrderBook{
    private:
        std::string symbol;
        std::priority_queue<Order,std::vector<Order>,std::function<bool(const Order&,const Order&)>> bids;
        std::priority_queue<Order,std::vector<Order>,std::function<bool(const Order&,const Order&)>> asks;
        std::vector<Trade> trades;
        std::mutex mtx;


    public:
        OrderBook(const std::string& symbol);
        void addOrder(float price, float quantity, std::string side, std::string type);
        std::vector<Trade>match();
};