# net

HTTP requests, on a worker thread, so the game never stalls.

Every URL a script touches is printed to the Script console before the request
starts, so nothing can phone home quietly. Only `http://` and `https://` are
accepted.

```lua
net.get("https://api.example.com/status", function(body)
    local data = json.decode(body)
    if data then client.notify("status: " .. tostring(data.state)) end
end)
```

## Starting a request

| Function | Returns |
| --- | --- |
| `net.get(url [, callback] [, headers])` | request handle |
| `net.post(url, body [, callback] [, headers])` | request handle |
| `net.request(options)` | request handle |

The callback receives the response body as a string; it runs on the main thread,
so it may touch the game, create nodes, draw, anything.

`net.request` takes a table:

```lua
local id = net.request {
    url      = "https://api.example.com/runs",
    method   = "POST",                       -- GET POST PUT PATCH DELETE HEAD
    body     = json.encode({ percent = 87 }),
    headers  = { ["Content-Type"] = "application/json" },
    timeout  = 30,                           -- seconds, 1..120
    callback = function(body) print(body) end,
}
```

## Checking a request without a callback

| Function | Returns |
| --- | --- |
| `net.done(handle)` | finished yet |
| `net.status(handle)` | `"pending"`, `"ok"` or `"error"` |
| `net.code(handle)` | HTTP status code |
| `net.body(handle)` | body, or `nil` while pending |
| `net.free(handle)` | forgets a finished request |
| `net.pending()` | how many are in flight |

This pairs nicely with [tasks](tasks.md):

```lua
task.run(function()
    local id = net.get("https://api.example.com/version")

    while not net.done(id) do task.wait() end

    if net.status(id) == "ok" then
        print(net.code(id), net.body(id))
    end

    net.free(id)
end)
```

## Helpers

`net.encode(text)` percent-escapes a string for a query parameter.

```lua
net.get("https://example.com/search?q=" .. net.encode("geometry dash"))
```

## Limits

* 8 requests in flight at once.
* 4 MB per request or response body.
* URLs up to 2048 characters, 16 headers per request.
* Timeouts 1..120 seconds, 20 by default.

When a script stops, its pending requests lose their callbacks, so nothing is
delivered into a state that no longer exists.
