import asyncio
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.books.books import order_books, locks
router = APIRouter()
clients = set()

@router.websocket("/marketdata")
async def get_market_data(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    try:
        while True:
            await asyncio.sleep(0.05)
            snapshot = order_books["BTC-USD"].get_snapshot(5)
            msg = {
                "snapshot": snapshot
            }
            await websocket.send_json(msg)
    except WebSocketDisconnect:
        clients.remove(websocket)
        print("Client disconnected")
