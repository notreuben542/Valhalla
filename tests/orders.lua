-- Lua script for wrk
math.randomseed(os.time())

request = function()
    local price = string.format("%.2f", 99 + math.random() * 2)
    local quantity = string.format("%.4f", 0.1 + math.random() * 1.9)
    local side = math.random() > 0.5 and "BUY" or "SELL"

    local body = string.format(
        '{"symbol":"BTC-USD","order_type":"LIMIT","side":"%s","price":"%s","quantity":"%s"}',
        side, price, quantity
    )

    return wrk.format("POST", "/api/v1/orders", {["Content-Type"] = "application/json"}, body)
end
