#include "include/order_book.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <functional>


static std::atomic<uint64_t> trade_id_counter{1}; // Atomic counter for trade IDs 


OrderBook::OrderBook(const std::string&sym):symbol(sym),bids([](const Order& a,const Order&b){

    if(a.price==b.price){
        return a.timestamp>b.timestamp; // Earlier timestamp has higher priority
    }
    return a.price<b.price; // Higher price has higher priority


}),asks([](const Order& a,const Order&b){

    if(a.price==b.price){
        return a.timestamp>b.timestamp; // Earlier timestamp has higher priority
    }
    return a.price>b.price; // Lower price has higher priority
})
{}


void OrderBook::addOrder(float price, float quantity, std::string side_str, std::string type_str){
    std::lock_guard<std::mutex> lock(mtx);
    static std::atomic<uint64_t> order_id_counter{1}; // Atomic counter for order IDs

    Side side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    OrderType type = (type_str == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;

    Order order(order_id_counter.fetch_add(1, std::memory_order_relaxed), side, type, price, quantity);
    if(order.side==Side::BUY){
        bids.push(order);}
    else{
        asks.push(order);}


    match();
}



//Matching engine

std::vector<Trade> OrderBook::match(){
    std::vector<Trade> newTrades;

    while(!bids.empty() && !asks.empty()){
        Order top_bid=bids.top();
        Order top_ask=asks.top();


        if(top_bid.price>=top_ask.price){
            double trade_price= top_ask.price;
            double trade_quantity= std::min(top_bid.quantity,top_ask.quantity);

            Trade trade;
            trade.trade_id = trade_id_counter.fetch_add(1, std::memory_order_relaxed);
            trade.symbol = symbol;
            trade.price = trade_price;
            trade.quantity = trade_quantity;
            trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            trade.maker_order_id = top_ask.order_id;
            trade.taker_order_id = top_bid.order_id;
            trade.aggressor_side = Side::BUY;

            newTrades.push_back(trade);
            trades.push_back(trade);

            // Update remaining quantities
            top_bid.quantity -= trade_quantity;
            top_ask.quantity -= trade_quantity;

            bids.pop();
            asks.pop();

            if (top_bid.quantity > 0) bids.push(top_bid);
            if (top_ask.quantity > 0) asks.push(top_ask);
        } else {
            break;
        }
    }

    return newTrades;
}