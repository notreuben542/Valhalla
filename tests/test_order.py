import asyncio
import httpx
import random
import time
import uuid
from collections import Counter

API_URL = "http://localhost:8000/api/v1/orders"
SYMBOL = "BTC-USD"
NUM_ORDERS = 50000
CONCURRENCY = 100  

async def send_order(client: httpx.AsyncClient, semaphore: asyncio.Semaphore):
    """Send a single order with concurrency control"""
    async with semaphore:
        side = random.choice(["buy", "sell"])
        order_type = "limit"
        price = round(random.uniform(99.0, 101.0), 2)
        quantity = round(random.uniform(0.1, 2.0), 4)
        
        payload = {
            "symbol": SYMBOL,
            "order_type": order_type,
            "side": side,
            "price": str(price),  # Convert to string for Decimal
            "quantity": str(quantity),  # Convert to string for Decimal
        }
        
        try:
            start = time.perf_counter()
            r = await client.post(API_URL, json=payload)
            latency = time.perf_counter() - start
            return {
                "status": r.status_code,
                "latency": latency,
                "error": None
            }
        except httpx.TimeoutException:
            return {"status": None, "latency": None, "error": "timeout"}
        except Exception as e:
            return {"status": None, "latency": None, "error": str(e)}

async def run_load_test():
    print(f"🚀 Starting load test: {NUM_ORDERS} orders with concurrency {CONCURRENCY}")
    
    # Create client with proper settings
    limits = httpx.Limits(
        max_keepalive_connections=CONCURRENCY,
        max_connections=CONCURRENCY * 2,
        keepalive_expiry=30.0
    )
    
    timeout = httpx.Timeout(
        connect=5.0,
        read=10.0,
        write=5.0,
        pool=10.0
    )
    
    async with httpx.AsyncClient(limits=limits, timeout=timeout) as client:
        # Warmup
        print("🔥 Warming up...")
        warmup_semaphore = asyncio.Semaphore(10)
        warmup_tasks = [send_order(client, warmup_semaphore) for _ in range(50)]
        await asyncio.gather(*warmup_tasks)
        print("✅ Warmup complete\n")
        
        # Main test
        semaphore = asyncio.Semaphore(CONCURRENCY)
        start = time.perf_counter()
        
        tasks = [send_order(client, semaphore) for _ in range(NUM_ORDERS)]
        results = await asyncio.gather(*tasks)
        
        elapsed = time.perf_counter() - start
        
        # Analyze results
        statuses = [r["status"] for r in results]
        latencies = [r["latency"] for r in results if r["latency"] is not None]
        errors = [r["error"] for r in results if r["error"] is not None]
        
        success_count = statuses.count(200)
        error_counts = Counter(errors)
        
        print(f"{'='*60}")
        print(f"📊 Load Test Results")
        print(f"{'='*60}")
        print(f"Total Orders:     {NUM_ORDERS}")
        print(f"Duration:         {elapsed:.2f}s")
        print(f"Throughput:       {NUM_ORDERS / elapsed:.2f} req/s")
        print(f"Success Rate:     {success_count}/{NUM_ORDERS} ({success_count/NUM_ORDERS*100:.1f}%)")
        
        if latencies:
            latencies.sort()
            print(f"\n📈 Latency Statistics:")
            print(f"  Min:     {min(latencies)*1000:.2f}ms")
            print(f"  Median:  {latencies[len(latencies)//2]*1000:.2f}ms")
            print(f"  P95:     {latencies[int(len(latencies)*0.95)]*1000:.2f}ms")
            print(f"  P99:     {latencies[int(len(latencies)*0.99)]*1000:.2f}ms")
            print(f"  Max:     {max(latencies)*1000:.2f}ms")
            print(f"  Avg:     {sum(latencies)/len(latencies)*1000:.2f}ms")
        
        if error_counts:
            print(f"\n❌ Errors:")
            for error, count in error_counts.most_common():
                print(f"  {error}: {count}")
        
        print(f"{'='*60}")

async def sustained_load_test(duration_seconds: int = 60, target_rps: int = 1000):
    """
    Sustained load test - maintains constant RPS for a duration.
    More realistic for testing steady-state performance.
    """
    print(f"🚀 Sustained load test: {target_rps} req/s for {duration_seconds}s")
    
    limits = httpx.Limits(max_keepalive_connections=200, max_connections=400)
    timeout = httpx.Timeout(connect=5.0, read=10.0, write=5.0, pool=10.0)
    
    async with httpx.AsyncClient(limits=limits, timeout=timeout) as client:
        start = time.perf_counter()
        order_interval = 1.0 / target_rps
        results = []
        
        order_count = 0
        while time.perf_counter() - start < duration_seconds:
            semaphore = asyncio.Semaphore(200)
            result = await send_order(client, semaphore)
            results.append(result)
            order_count += 1
            
            # Rate limiting
            await asyncio.sleep(order_interval)
        
        elapsed = time.perf_counter() - start
        success_count = sum(1 for r in results if r["status"] == 200)
        
        print(f"\n📊 Sustained Test Results:")
        print(f"Orders sent:      {order_count}")
        print(f"Actual duration:  {elapsed:.2f}s")
        print(f"Actual RPS:       {order_count/elapsed:.2f}")
        print(f"Success rate:     {success_count/order_count*100:.1f}%")

if __name__ == "__main__":
    # Run burst test
    # asyncio.run(run_load_test())
    
    # Uncomment for sustained test
    asyncio.run(sustained_load_test(duration_seconds=30, target_rps=1000))