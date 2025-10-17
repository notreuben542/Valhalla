# benchmark_live_ws_live_orders.py
import asyncio
import aiohttp
import websockets
import json
import random
import time
import statistics

HTTP_URL = "http://127.0.0.1:8000/api/v1/orders"
WS_URI = "ws://127.0.0.1:8000/api/v1/trades"

NUM_HTTP_CLIENTS = 50
ORDERS_PER_CLIENT = 200
NUM_WS_CLIENTS = 100
PREFILL_DURATION = 5      # seconds of aggressive prefill phase
LIVE_DURATION = 30        # seconds to keep listening after submissions finish

# Limit concurrent WS connects so we don't slam the server accept backlog
MAX_CONCURRENT_WS_CONNECTS = 20

metrics = {
    "http_orders_sent": 0,
    "http_errors": 0,
    "ws_messages_received": 0,
    "ws_errors": 0,
    "ws_latency": []
}


def build_order(client_id, i):
    return {
        "symbol": "BTC-USD",
        "side": "BUY" if (i % 2 == 0) else "SELL",
        "order_type": "LIMIT",
        "price": 100 + random.randint(0, 10),
        "quantity": random.randint(1, 10),
        "client_id": client_id,
    }



async def submit_orders_batch(client_id, session, orders_batch):
    """Send a batch of orders concurrently for a client."""
    global metrics

    async def send_order(order):
        try:
            async with session.post(HTTP_URL, json=order) as resp:
                if resp.status == 200:
                    metrics["http_orders_sent"] += 1
                else:
                    metrics["http_errors"] += 1
        except Exception:
            metrics["http_errors"] += 1

    tasks = [asyncio.create_task(send_order(order)) for order in orders_batch]
    await asyncio.gather(*tasks)


async def submit_orders(client_id, session, num_orders=ORDERS_PER_CLIENT):
    """Send num_orders in batches to maximize throughput."""
    BATCH_SIZE = 5 
    orders = [build_order(client_id, i) for i in range(num_orders)]
    
    # Send in batches
    for i in range(0, num_orders, BATCH_SIZE):
        batch = orders[i:i+BATCH_SIZE]
        await submit_orders_batch(client_id, session, batch)

async def ws_client(client_id, stop_event, connect_sem: asyncio.Semaphore):
    global metrics
    retry_backoff = 0.1
    max_backoff = 2.0

    await asyncio.sleep(min(0.05 * client_id, 2.0))  # stagger start

    while not stop_event.is_set():
        try:
            async with connect_sem:
                async with websockets.connect(
                    WS_URI,
                    ping_interval=10,
                    ping_timeout=5,
                    close_timeout=2,
                    max_size=2**20
                ) as ws:
                    retry_backoff = 0.1

                    while True:
                        if stop_event.is_set():
                            break  # stop recv before sending close

                        try:
                            msg = await asyncio.wait_for(ws.recv(), timeout=2.0)
                        except asyncio.TimeoutError:
                            continue
                        except websockets.exceptions.ConnectionClosedOK:
                            break
                        except websockets.exceptions.ConnectionClosedError:
                            metrics["ws_errors"] += 1
                            break

                        metrics["ws_messages_received"] += 1
                        metrics["ws_latency"].append(0)  # optionally measure latency

                    # Now safe to close after recv loop ends
                    if not ws.closed:
                        try:
                            await ws.close(code=1000, reason="client shutdown")
                        except Exception:
                            pass

        except Exception:
            metrics["ws_errors"] += 1
            if stop_event.is_set():
                break
            await asyncio.sleep(retry_backoff)
            retry_backoff = min(max_backoff, retry_backoff * 2)


async def prefill_orderbook(prefill_duration_seconds=5):
    """Aggressively prefill the book for `prefill_duration_seconds` seconds."""
    end_t = time.time() + prefill_duration_seconds
    async with aiohttp.ClientSession() as session:
        while time.time() < end_t:
            # spawn small batches to allow concurrency but avoid over-saturation
            tasks = [asyncio.create_task(submit_orders(i, session, num_orders=5))
                     for i in range(NUM_HTTP_CLIENTS)]
            await asyncio.gather(*tasks)


async def run_test():
    stop_event = asyncio.Event()

    #Prefill phase (warm-up)
    print(f"Prefilling order book for {PREFILL_DURATION}s ...")
    await prefill_orderbook(PREFILL_DURATION)
    print("Prefill complete.\n")

    # Start websocket listeners AFTER prefill (so they listen to live trades)
    print(f"Starting {NUM_WS_CLIENTS} WebSocket clients...")
    connect_sem = asyncio.Semaphore(MAX_CONCURRENT_WS_CONNECTS)
    ws_tasks = [asyncio.create_task(ws_client(i, stop_event, connect_sem))
                for i in range(NUM_WS_CLIENTS)]

  
    await asyncio.sleep(0.5)

    #Submit HTTP orders concurrently (live), while WS clients remain listening
    print("Submitting HTTP orders concurrently...")
    start_http = time.time()
    async with aiohttp.ClientSession() as session:
        http_tasks = [asyncio.create_task(submit_orders(i, session, ORDERS_PER_CLIENT))
                      for i in range(NUM_HTTP_CLIENTS)]
        await asyncio.gather(*http_tasks)
    http_duration = time.time() - start_http
    print(f"[+] HTTP order submission completed in {http_duration:.2f}s")
    print(f"    HTTP sent: {metrics['http_orders_sent']}, HTTP errors: {metrics['http_errors']}")

    # Keep WS clients alive for the live duration so they receive remaining broadcasts
    print(f"Listening for {LIVE_DURATION}s to allow streaming to continue...")
    await asyncio.sleep(LIVE_DURATION)

    
    stop_event.set()
    await asyncio.gather(*ws_tasks, return_exceptions=True)

    #Compute metrics
    ws_lat_sorted = sorted(metrics["ws_latency"])
    http_count = metrics["http_orders_sent"]
    ws_msgs = metrics["ws_messages_received"]
    ws_errs = metrics["ws_errors"]

    def pctile(arr, p):
        if not arr:
            return 0.0
        k = int(len(arr) * p / 100)
        return arr[min(k, len(arr)-1)]

    print("\n=== SUMMARY ===")
    print(f"HTTP orders sent: {http_count}, errors: {metrics['http_errors']}")
    print(f"HTTP throughput: {http_count / http_duration:.2f} orders/sec")
    print(f"WS messages received: {ws_msgs}, errors: {ws_errs}")
    if metrics["ws_latency"]:
        print(f"WS latency (s): avg={statistics.mean(metrics['ws_latency']):.6f}, "
              f"p50={pctile(ws_lat_sorted,50):.6f}, p95={pctile(ws_lat_sorted,95):.6f}")
    print(f"WS throughput: {metrics['ws_messages_received'] / (http_duration + LIVE_DURATION):.2f} msg/sec")
    print("=================\n")


if __name__ == "__main__":
    asyncio.run(run_test())
