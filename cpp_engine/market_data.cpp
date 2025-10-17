#include "include/order_book.hpp"



std::pair<std::pair<double, double>, std::pair<double, double>> OrderBook::getBBO() const{
    double bestbidprice = 0.0;
    double bestbidqty = 0.0;
    double bestaskprice = 0.0;
    double bestaskqty = 0.0;

    if(!bid_levels.empty()){
        bestbidprice = bid_levels.begin()->first;
        bestbidqty = bid_levels.begin()->second;
    }
    if(!ask_levels.empty()){
        bestaskprice = ask_levels.begin()->first;
        bestaskqty = ask_levels.begin()->second;
    }
    return {{bestbidprice, bestbidqty}, {bestaskprice, bestaskqty}};
}


nlohmann::json OrderBook::getSnapshot(size_t depth)const{

    nlohmann::json snapshot;
    snapshot["symbol"]= symbol;

    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()
    ).count() % 1000000;
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%FT%T") << "." << std::setw(6)
        << std::setfill('0') << us << "Z";
    snapshot["timestamp"] = oss.str();


    // BBO

    auto [bid, ask] = getBBO();
    snapshot["bbo"] = {
        {"bid", {{"price", bid  .first}, {"quantity", bid.second}}},
        {"ask", {{"price", ask.first}, {"quantity", ask.second}}}
    };


    // Top N bids

    nlohmann::json bids_array = nlohmann::json::array();
    size_t count = 0;
    for(const auto& [price, qty] : bid_levels){
        if(++count >= depth) break;
        bids_array.push_back({{"price", price}, {"quantity", qty}});
    }
    snapshot["bids"] = bids_array;


    // Top N asks
    nlohmann::json asks_array = nlohmann::json::array();
    count = 0;
    for (auto& [price, qty] : ask_levels) {
        asks_array.push_back({{"price", price}, {"quantity", qty}});
        if (++count >= depth) break;
    }

   
    snapshot["asks"] = asks_array;


    return snapshot;
    
}