#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>        
#include "include/order.hpp"
#include "include/order_book.hpp"
#include "include/trade.hpp"
#include "include/pybind11_json/include/pybind11_json/pybind11_json.hpp"

namespace py = pybind11;

PYBIND11_MODULE(matching_engine, m) {
    m.doc() = "C++ Matching Engine Module";

    py::enum_<Side>(m, "Side")
        .value("BUY", Side::BUY)
        .value("SELL", Side::SELL)
        .export_values();

    py::enum_<OrderType>(m, "OrderType")
        .value("LIMIT", OrderType::LIMIT)
        .value("MARKET", OrderType::MARKET)
        .export_values();

    py::class_<Order>(m, "Order")
        .def(py::init<uint64_t, Side, OrderType, double, double>())  // assuming price & quantity are double
        .def_readonly("order_id", &Order::order_id)
        .def_readonly("side", &Order::side)
        .def_readonly("type", &Order::type)
        .def_readonly("price", &Order::price)
        .def_readonly("quantity", &Order::quantity)
        .def_readonly("timestamp", &Order::timestamp);

    py::class_<Trade>(m, "Trade")
        .def_readonly("trade_id", &Trade::trade_id)
        .def_readonly("symbol", &Trade::symbol)
        .def_readonly("price", &Trade::price)
        .def_readonly("quantity", &Trade::quantity)
        .def_readonly("timestamp", &Trade::timestamp)
        .def_readonly("maker_order_id", &Trade::maker_order_id)
        .def_readonly("taker_order_id", &Trade::taker_order_id)
        .def_readonly("aggressor_side", &Trade::aggressor_side);

    py::class_<OrderBook>(m, "OrderBook")
        .def(py::init<const std::string&>())
        .def("add_order", &OrderBook::addOrder)
        .def("limit_order", &OrderBook::limitOrder)
        .def("market_order", &OrderBook::marketOrder)
        .def_readwrite("trade_callback", &OrderBook::trade_callback)
        .def("get_bbo", &OrderBook::getBBO)
        .def("get_snapshot",[](OrderBook&ob, size_t depth){
            nlohmann::json snap = ob.getSnapshot(depth);
            return snap;
        });
        
}
