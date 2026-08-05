# Neverhook Lua API

Neverhook runs scripts on **LuaJIT 2.1** (Lua 5.1 syntax plus the `bit` library).

A script is a plain `.lua` text file. Drop it in the scripts folder, open the
**Scripts** tab, press **Load**. If the script builds a tab of its own, a
**Scripts** group appears in the left sidebar with that tab underneath it.

```lua
local tab = menu.tab("Hello")
tab:toggle("on", "Print every frame", false)

callbacks.add("frame", function()
    if tab:get("on") then
        print("tick")
    end
 end)
```

## What a script can reach

| Table | What it does |
| --- | --- |
| [`callbacks`](api/callbacks.md) | Subscribe to frame, draw, level and death events |
| [`hooks`](api/hooks.md) | Sit inside a game function, read its arguments, cancel it |
| [`menu`](api/menu.md) | Build your own tab with toggles, sliders, combos and buttons |
| [`gd`](api/gd.md) | Read the level and the player, move the player |
| [`mod`](api/mod.md) | Flip Neverhook's own features by name |
| [`draw`](api/draw.md) | Draw text and shapes on top of the game |
| [`client`](api/client.md) | Logging, frame time, key state |
| [`storage`](api/storage.md) | Save settings between sessions |
| [`utils`](api/utils.md) | Small maths helpers |

Plus the standard `string`, `table`, `math`, `bit`, `coroutine`, and a trimmed
`os` (`clock`, `time`, `date`, `difftime`).

## What a script cannot reach

No `ffi`, no `io`, no `package`/`require`, no `load`/`loadstring`/`dofile`, no
`debug`, no raw memory access, no filesystem paths, and no arbitrary network
access — the only thing that leaves the machine is
[`draw.image_url`](api/images.md), which fetches an image and nothing else. This
is not a restriction the API forgot to lift — it is the point. See
[Sandbox and security](security.md).
