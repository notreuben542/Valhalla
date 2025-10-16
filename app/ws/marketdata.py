import asyncio
from fastapi import FASTAPI, WebSocketDisconnect,Websocket
import matching_engine

app = FASTAPI()
ob = matching_engine.OrderBook()
clients = set()

@app.websocket("/ws/marketdata")
async def getMarketData(websocket: Websocket):
    await websocket.accept()
    clients.add(websocket)
    try:
        while True:
           await asyncio.sleep(0.05)
           bbo = ob.get_bbo()
           snapshot = ob.get_snapshot(5)
           msg = ({
               "bbo":bbo,
                "snapshot":snapshot
           })
           await websocket.send_json(msg)
    except WebSocketDisconnect:
        clients.remove(websocket)

