from fastapi import APIRouter, Request
from decimal import Decimal
import matching_engine

router = APIRouter()

# Maintain one order book per symbol (C++ object)
order_books: dict[str, matching_engine.OrderBook] = {}


@router.post("/orders")
async def submit_order(order_request: Request):
    body = await order_request.json()
    symbol = body["symbol"]
    side = body["side"].upper()  # "BUY" or "SELL"
    order_type = body["order_type"].upper()  # "LIMIT" or "MARKET"
    price = float(body.get("price", 0))  
    quantity = float(body["quantity"])

    # Get or create order book for the symbol
    if symbol not in order_books:
        order_books[symbol] = matching_engine.OrderBook(symbol)
    book = order_books[symbol]

    trades = book.add_order(price, quantity,side,order_type)

    
    trade_list = []
    if trades:
        for t in trades:
            trade_list.append({
                "price": t.price,
                "quantity": t.quantity,
                "timestamp": t.timestamp,
                "maker_order_id": t.maker_order_id,
                "taker_order_id": t.taker_order_id,
                "aggressor_side": t.aggressor_side,
            })

    return {
        "status": "success",
        "symbol": symbol,
        "trades": trade_list
    }
