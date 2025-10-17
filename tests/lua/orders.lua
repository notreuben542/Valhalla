-- High-RPS optimized wrk Lua script
-- Random orders for BTC-USD market
math.randomseed(os.time())

-- Precompute static JSON parts
local static_prefix = '{"symbol":"BTC-USD","order_type":"MARKET","side":"'
local static_middle = '","price":"'
local static_suffix = '","quantity":"'

-- Pre-generate side choices to reduce math.random calls
local sides = {"BUY", "SELL"}
local side_index = 1

-- Random number helpers
local function rand_price()
    return string.format("%.2f", 99 + math.random() * 2)
end

local function rand_quantity()
    return string.format("%.4f", 0.1 + math.random() * 1.9)
end

request = function()
    -- Alternate sides instead of calling math.random every time
    side_index = 3 - side_index  -- flips between 1 and 2
    local side = sides[side_index]

    local price = rand_price()
    local quantity = rand_quantity()

    -- Construct JSON body using concatenation (faster than string.format)
    local body = static_prefix .. side .. static_middle .. price .. static_suffix .. quantity .. '"}'

    return wrk.format("POST", "/api/v1/orders", {["Content-Type"] = "application/json"}, body)
end
