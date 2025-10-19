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

        # Handle batch orders
        if isinstance(body, list):
            results = []
            for order in body:
                result = await _process_single_order(order)
                results.append(result)
            return {"status": "success", "orders": results}

        # Handle single order
        elif isinstance(body, dict):
            result = await _process_single_order(body)
            return {"status": "success", "order": result}

        else:
            logger.warning("Invalid request body type: %s", type(body))
            return {"status": "error", "message": "Invalid request body."}

    except Exception as e:
        logger.error("Error processing order: %s, exception: %s", body, e, exc_info=True)
        return {"status": "error", "message": str(e)}

async def _process_single_order(order: dict):
    """Validate and submit a single order to the matching engine."""
    symbol = order.get("symbol")
    side = order.get("side", "").upper()
    order_type = order.get("order_type", "").upper()
    price = float(order.get("price", 0))
    quantity = float(order.get("quantity", 0))

    # Validation
    if symbol != "BTC-USD":
        logger.warning("Invalid or missing symbol: %s", order)
        return {"status": "error", "message": "Invalid or missing symbol."}
    if price <= 0 or quantity <= 0:
        logger.warning("Invalid order parameters: %s", order)
        return {"status": "error", "message": "Price and quantity must be positive numbers."}
    if order_type not in ["LIMIT", "MARKET", "IOC", "FOK"]:
        logger.warning("Invalid order type: %s", order)
        return {"status": "error", "message": "Invalid order type."}
    if side not in ["BUY", "SELL"]:
        logger.warning("Invalid order side: %s", order)
        return {"status": "error", "message": "Invalid order side."}

    # Get or create order book
    book = order_books.setdefault(symbol, matching_engine.OrderBook(symbol))

    # Submit order
    trades = book.add_order(price, quantity, side, order_type)
    return {"status": "success", "symbol": symbol, "trades_executed": len(trades)}
