import asyncio
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.books.books import order_books
import logging

router = APIRouter()
logger = logging.getLogger(__name__)

clients = set()

@router.websocket("/marketdata")
async def get_market_data(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    logger.info("Client connected: %s", websocket.client)

    try:
        while True:
            await asyncio.sleep(0.05)  # Send updates every 50ms
            book = order_books.get("BTC-USD")
            if not book:
                logger.error("Order book for BTC-USD not found")
                await websocket.close()
                clients.discard(websocket)
                return

            snapshot = book.get_snapshot(5)
            msg = {"snapshot": snapshot}

          
            await websocket.send_json(msg)
            
    except WebSocketDisconnect:
        clients.discard(websocket)
        logger.info("Client disconnected: %s", websocket.client)
    except Exception as e:
        clients.discard(websocket)
        logger.error("WebSocket error for client %s: %s", websocket.client, e)
