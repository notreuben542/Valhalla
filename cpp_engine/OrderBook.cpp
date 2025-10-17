#include "include/order_book.hpp"
#include <iostream>
#include <chrono>
#include <atomic>
#include <functional>


static std::atomic<uint64_t> trade_id_counter{1}; // Atomic counter for trade IDs
std::atomic<uint64_t> OrderBook::order_id_counter{1}; // Atomic counter for order IDs

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


std::vector<Trade> OrderBook::addOrder(double price, double quantity, std::string side_str, std::string type_str){
    std::lock_guard<std::mutex> lock(mtx);

    std::vector<Trade> executed_trades;
    Side side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    OrderType type = (type_str == "LIMIT") ? OrderType::LIMIT : OrderType::MARKET;

    Order order(order_id_counter.fetch_add(1, std::memory_order_relaxed), side, type, price, quantity);

    if(type == OrderType::MARKET){
       executed_trades = marketOrder(order);
    }
    else if(type == OrderType::IOC){
       executed_trades = iocOrder(order);
    }
    else if(type == OrderType::FOK){
       executed_trades = fokOrder(order);
    }
    else {
        // Limit order
        if (order.side == Side::BUY) {
            bids.push(order);
            bid_levels[price] += quantity;
        } else {
            asks.push(order);
            ask_levels[price] += quantity;
        }
        executed_trades = limitOrder(order);
        

       for(const auto& trade: executed_trades){
            updateLevel(Side::BUY, trade.price, -trade.quantity);
            updateLevel(Side::SELL, trade.price, -trade.quantity);
       }
    }
      return executed_trades;
     
}



//Matching engine

std::vector<Trade> OrderBook::limitOrder(Order& order){
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
            trade.aggressor_side = (order.side == Side::BUY) ? Side::BUY : Side::SELL;

            if (order.side == Side::BUY) {
                trade.maker_fee = calculateFee(Side::SELL, trade_quantity * trade_price);
                trade.taker_fee = calculateFee(Side::BUY, trade_quantity * trade_price);
            } else {
                trade.maker_fee = calculateFee(Side::BUY, trade_quantity * trade_price);
                trade.taker_fee = calculateFee(Side::SELL, trade_quantity * trade_price);
            }

            newTrades.push_back(trade);
            trades.push_back(trade);


            if(trade_callback){
                trade_callback(trade);
            }

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

            trade.maker_fee = calculateFee(Side::SELL, trade_quantity * trade_price);
            trade.taker_fee = calculateFee(Side::BUY, trade_quantity * trade_price);

            trades.push_back(trade);
            trades_executed.push_back(trade);
            if(trade_callback){
                trade_callback(trade);
            }
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

            trade.maker_fee = calculateFee(Side::BUY, trade_quantity * trade_price);
            trade.taker_fee = calculateFee(Side::SELL, trade_quantity * trade_price);



            trades.push_back(trade);
            trades_executed.push_back(trade);
            if(trade_callback){
                trade_callback(trade);
            }

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

std::vector<Trade> OrderBook::iocOrder(Order& order) {
    std::vector<Trade> trades_executed;

    if(order.side == Side::BUY) {
        while(order.quantity > 0 && !asks.empty()) {
            Order top_ask = asks.top();
            if(top_ask.price > order.price) break; 

            double trade_qty = std::min(order.quantity, top_ask.quantity);
            double trade_price = top_ask.price;

            Trade trade;
            trade.trade_id = trade_id_counter.fetch_add(1, std::memory_order_relaxed);
            trade.symbol = symbol;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            trade.maker_order_id = top_ask.order_id;
            trade.taker_order_id = order.order_id;
            trade.aggressor_side = Side::BUY;

            trades.push_back(trade);
            trades_executed.push_back(trade);

            order.quantity -= trade_qty;
            top_ask.quantity -= trade_qty;

            asks.pop();
            if(top_ask.quantity > 0) asks.push(top_ask);

            updateLevel(Side::SELL, trade_price, -trade_qty);
        }
    } else { // SELL
        while(order.quantity > 0 && !bids.empty()) {
            Order top_bid = bids.top();
            if(top_bid.price < order.price) break; 

            double trade_qty = std::min(order.quantity, top_bid.quantity);
            double trade_price = top_bid.price;

            Trade trade;
            trade.trade_id = trade_id_counter.fetch_add(1, std::memory_order_relaxed);
            trade.symbol = symbol;
            trade.price = trade_price;
            trade.quantity = trade_qty;
            trade.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()
            ).count();
            trade.maker_order_id = top_bid.order_id;
            trade.taker_order_id = order.order_id;
            trade.aggressor_side = Side::SELL;

            trades.push_back(trade);
            trades_executed.push_back(trade);

            order.quantity -= trade_qty;
            top_bid.quantity -= trade_qty;

            bids.pop();
            if(top_bid.quantity > 0) bids.push(top_bid);

            updateLevel(Side::BUY, trade_price, -trade_qty);
        }
    }

    return trades_executed;
}

std::vector<Trade> OrderBook::fokOrder(Order& order) {
    std::vector<Trade> trades_executed;

   
    double available_qty = 0.0;

    if(order.side == Side::BUY) {
        for(auto& [price, qty] : ask_levels) {
            if(price > order.price) break;
            available_qty += qty;
            if(available_qty >= order.quantity) break;
        }
        if(available_qty < order.quantity) return trades_executed; 
        trades_executed = iocOrder(order); 
    } else { // SELL
        for(auto& [price, qty] : bid_levels) {
            if(price < order.price) break;
            available_qty += qty;
            if(available_qty >= order.quantity) break;
        }
        if(available_qty < order.quantity) return trades_executed;
        trades_executed = iocOrder(order); 
    }

    return trades_executed;
}



// helper function to calculate maker-taker fees

double OrderBook::calculateFee(Side side, double amount) const {
    if (side == Side::BUY) {
        return amount * taker_fee_rate; // Assuming BUY orders are taker orders
    } else {
        return amount * maker_fee_rate; // Assuming SELL orders are maker orders
    }
}