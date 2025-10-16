import asyncio
import websockets

NUM_CLIENTS = 10 

async def ws_client(client_id: int):
    uri = "ws://127.0.0.1:8000/ws/marketdata"
    try:
        async with websockets.connect(uri) as websocket:
            for _ in range(10):  # receive 10 messages
                msg = await websocket.recv()
                if client_id % 100 == 0:
                    print(f"Client {client_id} received message")
    except Exception as e:
        print(f"Client {client_id} error: {e}")

async def main():
    tasks = [asyncio.create_task(ws_client(i)) for i in range(NUM_CLIENTS)]
    await asyncio.gather(*tasks)

asyncio.run(main())
