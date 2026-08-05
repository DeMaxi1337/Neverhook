# storage

Saves values between sessions. Data goes into Neverhook's own save file under a
`script.<script name>.` prefix, so scripts cannot read each other's keys and no
filesystem path is ever handed to Lua.

Only booleans, numbers and strings can be stored. Tables are not supported —
encode them yourself if you need one.

## storage.set(key, value)

```lua
storage.set("enabled", true)
storage.set("power", 42)
storage.set("mode", "aggressive")
```

Passing anything other than a boolean, number or string is an error.

## storage.get(key, default)

Reads a value back. **The default decides the type**, so always pass one of the
right kind:

```lua
local enabled = storage.get("enabled", false)   -- boolean
local power   = storage.get("power", 0)         -- number
local mode    = storage.get("mode", "normal")   -- string
```

With no default, or with a `nil` one, you get `nil` back — the runtime has no
way to guess what type the key holds.

## Typical use

Restore at load time, save when the value changes:

```lua
local tab = menu.tab("Saved demo")
tab:toggle("enabled", "Enabled", false)
tab:slider("power", "Power", 0, 100, 50)

tab:set("enabled", storage.get("enabled", false))
tab:set("power",   storage.get("power", 50))

local lastEnabled = tab:get("enabled")
local lastPower   = tab:get("power")

callbacks.add("frame", function()
    local enabled = tab:get("enabled")
    local power   = tab:get("power")

    if enabled ~= lastEnabled then
        storage.set("enabled", enabled)
        lastEnabled = enabled
    end

    if power ~= lastPower then
        storage.set("power", power)
        lastPower = power
    end
end)
```

Writing only on change keeps the save file from being rewritten every frame.
