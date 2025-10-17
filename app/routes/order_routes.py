from fastapi import APIRouter, Request
from app.books.books import order_books
import matching_engine
import logging

router = APIRouter()
logger = logging.getLogger(__name__) 

@router.post("/orders")
async def submit_order(order_request: Request):
    try:
        body = await order_request.json()
        symbol = body.get("symbol")
        side = body.get("side", "").upper()
        order_type = body.get("order_type", "").upper()
        price = float(body.get("price", 0))
        quantity = float(body.get("quantity", 0))

        # Validation
        if symbol != "BTC-USD":
            logger.warning("Invalid or missing symbol: %s", body)
            return {"status": "error", "message": "Invalid or missing symbol."}
        if price <= 0 or quantity <= 0:
            logger.warning("Invalid order parameters: %s", body)
            return {"status": "error", "message": "Price and quantity must be positive numbers."}

        if order_type not in ["LIMIT", "MARKET", "IOC", "FOK"]:
            logger.warning("Invalid order type: %s", body)
            return {"status": "error", "message": "Invalid order type."}

        if side not in ["BUY", "SELL"]:
            logger.warning("Invalid order side: %s", body)
            return {"status": "error", "message": "Invalid order side."}

        if symbol not in order_books:
            logger.warning("Unsupported trading pair: %s", symbol)
            return {"status": "error", "message": "Unsupported trading pair."}

        # Get or create order book
        book = order_books.setdefault(symbol, matching_engine.OrderBook(symbol))

        # Submit order
        trades = book.add_order(price, quantity, side, order_type)
        logger.info("Order submitted: %s, executed trades: %d", body, len(trades))

        return {"status": "success", "symbol": symbol, "trades": trades}

    except Exception as e:
        logger.error("Error processing order: %s, exception: %s", body, e, exc_info=True)
        return {"status": "error", "message": str(e)}
