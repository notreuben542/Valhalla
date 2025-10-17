from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.books.books import order_books
import asyncio
import logging

router = APIRouter()
logger = logging.getLogger(__name__)

clients = set()

@router.websocket("/trades")
async def get_trade_data(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    logger.info("Client connected: %s", websocket.client)

    loop = asyncio.get_event_loop()

    # Callback for new trades
    def on_trade(trade):
        trade_data = {
            "trade_id": trade.trade_id,
            "symbol": trade.symbol,
            "price": trade.price,
            "quantity": trade.quantity,
            "timestamp": trade.timestamp,
            "maker_order_id": trade.maker_order_id,
            "taker_order_id": trade.taker_order_id,
            "aggressor_side": trade.aggressor_side.name,
        }

      
        for client in clients.copy():
            if client.client_state.name != "CONNECTED":
                clients.discard(client)
                continue
            loop.create_task(send_trade(client, trade_data))

    async def send_trade(client: WebSocket, trade_data: dict):
        try:
            await client.send_json({"trade": trade_data})
        except Exception as e:
            logger.warning("Failed to send trade to client %s: %s", client.client, e)
            clients.discard(client)

    # Assign the callback
    order_books.trade_callback = on_trade

    try:
        while True:
            await asyncio.sleep(0.05)  # Keep the WebSocket alive
    except WebSocketDisconnect:
        clients.discard(websocket)
        logger.info("Client disconnected: %s", websocket.client)
    except Exception as e:
        clients.discard(websocket)
        logger.error("WebSocket error for client %s: %s", websocket.client, e)
