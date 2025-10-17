from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.books.books import order_books, locks
import asyncio

router = APIRouter()


clients = set()

@router.websocket("/trades")
async def get_trade_data(websocket: WebSocket):
    await websocket.accept()
    clients.add(websocket)
    loop = asyncio.get_event_loop()

    book = order_books.get("BTC-USD")
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
        # Send Synchronously to the connected  clients
        
        for client in clients:
            loop.create_task(client.send_json({"trade": trade_data}))



    #Assign the callback
    book.trade_callback = on_trade
    try:
        while True:
            await asyncio.sleep(0.05)
    except WebSocketDisconnect:
        clients.remove(websocket)
        print("Client disconnected")

        
            
