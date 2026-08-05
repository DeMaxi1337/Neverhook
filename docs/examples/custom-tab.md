# A tab with a working toggle

The smallest script that is actually useful: it adds a tab to the sidebar, puts
two controls in it, and does something with them.

Save as `bhop.lua` in the scripts folder and press **Load**. A **Scripts** group
appears in the left sidebar with a **Bhop** entry under it.

```lua
-- bhop.lua -- holds the player slightly airborne while enabled

local tab = menu.tab("Bhop")

tab:label("Nudges the player upward while it is falling.")
tab:separator()
tab:toggle("enabled", "Enabled", false)
tab:slider("force", "Force", 0.5, 10.0, 3.0)
tab:button("Reset force", function()
    tab:set("force", 3.0)
    print("force reset")
end)

-- remember the toggle between sessions
tab:set("enabled", storage.get("enabled", false))
local lastEnabled = tab:get("enabled")

callbacks.add("frame", function()
    local enabled = tab:get("enabled")

    if enabled ~= lastEnabled then
        storage.set("enabled", enabled)
        lastEnabled = enabled
    end

    if not enabled then return end
    if not gd.in_level() then return end

    local p = gd.player()
    if not p or p.dead then return end

    if p.y_velocity < 0 then
        gd.set_player_pos(p.x, p.y + tab:get("force") * 60 * client.frame_time())
    end
end)
```

## What to notice

* **The UI is built once**, at the top of the file. Calling `menu.tab` inside a
  callback would add a new sidebar entry every frame.
* **`tab:get` is the source of truth.** The widget the user clicks and the value
  the script reads are the same thing, so there is no state to keep in sync.
* **Guards come first.** `gd.in_level()` and the `p` nil check mean the script
  is harmless in the menu, in the editor, and on the loading screen.
* **`client.frame_time()`** keeps the nudge the same at 60 and 240 fps.
* Stopping the script removes the tab and the callback in one go.
