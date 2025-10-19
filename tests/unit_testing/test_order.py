import asyncio
import httpx
import random
import time
import numpy as np

API_URL = "http://localhost:8000/api/v1/orders"
SYMBOL = "BTC-USD"

NUM_ORDERS = 100_000         # total orders to send
BATCH_SIZE = 5000            # orders per HTTP request
CONCURRENT_BATCHES = 20      # number of batches to send in parallel

metrics = {
    "orders_sent": 0,
    "errors": 0,
    "latencies": []
}

def generate_order():
    """Generate a random LIMIT order."""
    side = random.choice(["BUY", "SELL"])
    price = round(random.uniform(99, 101), 2)
    quantity = round(random.uniform(0.1, 2.0), 4)
    return {
        "symbol": SYMBOL,
        "side": side,
        "order_type": "LIMIT",
        "price": str(price),
        "quantity": str(quantity)
    }

async def send_batch(session: httpx.AsyncClient, orders_batch):
    """Send a batch of orders and record latencies."""
    start = time.perf_counter()
    try:
        r = await session.post(API_URL, json=orders_batch)
        latency = time.perf_counter() - start
        metrics["latencies"].extend([latency] * len(orders_batch))
        if r.status_code == 200:
            metrics["orders_sent"] += len(orders_batch)
        else:
            metrics["errors"] += len(orders_batch)
    except Exception:
        metrics["errors"] += len(orders_batch)

async def main():
    # Pre-generate all orders
    orders = [generate_order() for _ in range(NUM_ORDERS)]
    batches = [orders[i:i+BATCH_SIZE] for i in range(0, NUM_ORDERS, BATCH_SIZE)]

    async with httpx.AsyncClient(
        timeout=httpx.Timeout(60.0, read=60.0),
        limits=httpx.Limits(max_connections=200, max_keepalive_connections=100)
    ) as client:

        start = time.perf_counter()
        # Send batches concurrently in groups
        for i in range(0, len(batches), CONCURRENT_BATCHES):
            batch_group = batches[i:i+CONCURRENT_BATCHES]
            tasks = [asyncio.create_task(send_batch(client, b)) for b in batch_group]
            await asyncio.gather(*tasks)
        elapsed = time.perf_counter() - start

    print(f"Orders sent: {metrics['orders_sent']}")
    print(f"Errors: {metrics['errors']}")
    print(f"Elapsed time: {elapsed:.2f}s")
    print(f"Throughput: {metrics['orders_sent']/elapsed:.2f} orders/sec")

    if metrics["latencies"]:
        latencies = np.array(metrics["latencies"])
        print(f"Avg latency: {latencies.mean():.6f}s")
        print(f"P50 latency: {np.percentile(latencies, 50):.6f}s")
        print(f"P95 latency: {np.percentile(latencies, 95):.6f}s")
        print(f"P99 latency: {np.percentile(latencies, 99):.6f}s")
        print(f"Max latency: {latencies.max():.6f}s")

if __name__ == "__main__":
    asyncio.run(main())
