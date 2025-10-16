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


void OrderBook::addOrder(double price, double quantity, std::string side_str, std::string type_str){
    std::lock_guard<std::mutex> lock(mtx);
    static std::atomic<uint64_t> order_id_counter{1}; // Atomic counter for order IDs

    Side side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    OrderType type = (type_str == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;

    Order order(order_id_counter.fetch_add(1, std::memory_order_relaxed), side, type, price, quantity);

    if(type == OrderType::MARKET){
        marketOrder(order);
        return;
    }
    else {
        // Limit order
        if (order.side == Side::BUY) bids.push(order);
        else asks.push(order);
       auto matched_trades = match();

       for(const auto& trade: matched_trades){
            updateLevel(Side::BUY, trade.price, -trade.quantity);
            updateLevel(Side::SELL, trade.price, -trade.quantity);
       }
    }

     
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


std::vector<Trade> OrderBook::marketOrder(Order& order){
    std::vector<Trade> trades_executed;
       if(order.side == Side::BUY){
        while(order.quantity>0 && !asks.empty()){
            Order top_ask = asks.top();
            double trade_quantity = std::min(order.quantity, top_ask.quantity);
            double trade_price = top_ask.price;


            Trade trade;
            trade.trade_id = trade_id_counter.fetch_add(1, std::memory_order_relaxed);
            trade.symbol = symbol;
            trade.price = trade_price;
            trade.quantity = trade_quantity;
            trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            trade.maker_order_id = top_ask.order_id;
            trade.taker_order_id = order.order_id;
            trade.aggressor_side = Side::BUY;

            trades.push_back(trade);
            trades_executed.push_back(trade);
            order.quantity -= trade_quantity;
            top_ask.quantity -= trade_quantity;

            asks.pop();
            if (top_ask.quantity > 0) asks.push(top_ask);
            updateLevel(Side::SELL, trade_price, -trade_quantity);
        }
       }
       else{
        while(order.quantity>0 && !bids.empty()){
            Order top_bid = bids.top();
            double trade_quantity = std::min(order.quantity,top_bid.quantity);
            double trade_price = top_bid.price;


            Trade trade;
            trade.trade_id = trade_id_counter.fetch_add(1, std::memory_order_relaxed);
            trade.symbol = symbol;
            trade.price = trade_price;
            trade.quantity = trade_quantity;
            trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            trade.maker_order_id = top_bid.order_id;
            trade.taker_order_id = order.order_id;
            trade.aggressor_side = Side::SELL;



            trades.push_back(trade);
            trades_executed.push_back(trade);

            order.quantity -= trade_quantity;
            top_bid.quantity -= trade_quantity;
            bids.pop();
            if (top_bid.quantity > 0) bids.push(top_bid);
            
            updateLevel(Side::BUY, trade_price, -trade_quantity);
        }
       }
       return trades_executed;
}

// Update order book levels
void OrderBook::updateLevel(Side side, double price, double qtychange) {
    if (side == Side::BUY) {
        bid_levels[price] += qtychange;
        if (bid_levels[price] <= 0)
            bid_levels.erase(price);
    } else {
        ask_levels[price] += qtychange;
        if (ask_levels[price] <= 0)
            ask_levels.erase(price);
    }
}


