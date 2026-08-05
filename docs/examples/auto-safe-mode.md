# Auto safe mode on death

Reacts to game events instead of polling every frame: after a few deaths in a
row it turns Safe Mode on, and clears the counter when you complete the level.

```lua
-- autosafe.lua

local tab = menu.tab("Auto Safe")
tab:toggle("enabled", "Enabled", false)
tab:slider_int("threshold", "Deaths before Safe Mode", 1, 20, 5)
tab:label("Counts deaths in the current level.")

local deaths = 0

local function announce(text)
    print(text)
end

callbacks.add("level_start", function()
    deaths = 0
end)

callbacks.add("death", function()
    if not tab:get("enabled") then return end

    deaths = deaths + 1

    if deaths == tab:get("threshold") and not mod.get("Safe Mode") then
        mod.set("Safe Mode", true)
        announce("safe mode on after " .. deaths .. " deaths")
    end
end)

callbacks.add("level_end", function()
    if not tab:get("enabled") then return end

    if mod.get("Safe Mode") then
        mod.set("Safe Mode", false)
        announce("level finished, safe mode off")
    end

    deaths = 0
end)

callbacks.add("draw", function()
    if not tab:get("enabled") or not gd.in_level() then return end

    local w = draw.screen_size()
    local text = "deaths " .. deaths .. "/" .. tab:get("threshold")
    local tw = draw.text_size(text)

    draw.text(w / 2 - tw / 2, 12, text, 200, 200, 210, 180)
end)
```

## What to notice

* **Events beat polling.** `"death"` fires once, so the counter is exact. Doing
  the same with `gd.player().dead` in `"frame"` would count one death many
  times and force you to track edges by hand.
* **`level_start` resets the state**, so the counter never leaks between levels.
* **`mod.get` before `mod.set`** keeps the script from fighting the user if they
  already switched Safe Mode on themselves.
* `deaths` is a plain local at file scope. Each script has its own VM, so there
  is nothing to collide with — no prefixes or namespaces needed.
