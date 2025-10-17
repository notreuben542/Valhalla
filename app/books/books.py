# books.py
import matching_engine
import asyncio

# Shared dict: symbol -> OrderBook
order_books: dict[str, matching_engine.OrderBook] = {}

# Async lock per symbol for thread-safety
locks: dict[str, asyncio.Lock] = {}
