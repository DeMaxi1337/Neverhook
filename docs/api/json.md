# json

A small JSON codec, useful for storing structured data with `storage` or for
reading a file you downloaded yourself.

## json.encode(value)

Turns a Lua value into a JSON string. Tables whose keys are exactly `1..n`
become arrays, everything else becomes an object.

```lua
local text = json.encode({ name = "run", best = 87, tags = { "fast", "safe" } })
-- {"name":"run","best":87,"tags":["fast","safe"]}
```

Returns `nil, error` when the value cannot be encoded (functions, userdata and
threads have no JSON form). Keys that are not strings or numbers are skipped.

## json.decode(text)

Parses a JSON string and returns the Lua value, or `nil, error` when the text is
not valid JSON.

```lua
local data, err = json.decode(text)
if not data then
    client.error(err)
    return
end

client.log(data.name .. " " .. tostring(data.best))
```

## Notes

- `null` becomes `nil`. Inside an array that means the array stops there, which
  is normal Lua behaviour.
- Nesting is limited to 64 levels when decoding and 32 when encoding.
- Documents larger than 4 MB are rejected.
- `\uXXXX` escapes are decoded into UTF-8. Lone surrogate halves become the
  replacement character.

## Storing a table

```lua
local function save(t)
    storage.set("state", json.encode(t))
end

local function load()
    local raw = storage.get("state")
    if type(raw) ~= "string" then return {} end
    return json.decode(raw) or {}
end
```
