# Speed HUD

Draws a small panel with the current percent, attempt time and vertical speed.
Shows how `"draw"` works and how to lay text out without hardcoding positions.

```lua
-- speedhud.lua

local tab = menu.tab("Speed HUD")
tab:toggle("enabled", "Enabled", true)
tab:combo("corner", "Corner", { "Top left", "Top right", "Bottom left" }, 1)
tab:toggle("hide_in_menu", "Hide while the menu is open", true)

local PANEL_W, PANEL_H, MARGIN = 190, 74, 20

local function panelOrigin()
    local sw, sh = draw.screen_size()
    local corner = tab:get("corner")

    if corner == 2 then return sw - PANEL_W - MARGIN, MARGIN end
    if corner == 3 then return MARGIN, sh - PANEL_H - MARGIN end
    return MARGIN, MARGIN
end

callbacks.add("draw", function()
    if not tab:get("enabled") then return end
    if tab:get("hide_in_menu") and client.menu_open() then return end
    if not gd.in_level() then return end

    local lvl = gd.level()
    local p   = gd.player()
    if not lvl or not p then return end

    local x, y = panelOrigin()

    draw.rect_filled(x, y, PANEL_W, PANEL_H, 6, 9, 16, 210, 6)
    draw.rect(x, y, PANEL_W, PANEL_H, 255, 255, 255, 18)
    draw.rect_filled(x, y, 3, PANEL_H, 77, 125, 255, 255, 2)

    local rows = {
        { "percent", string.format("%.2f%%", lvl.percent) },
        { "time",    string.format("%.2fs", gd.time())    },
        { "y speed", string.format("%.1f", p.y_velocity)  },
    }

    for i, row in ipairs(rows) do
        local ry = y + 10 + (i - 1) * 18
        draw.text(x + 14, ry, row[1], 130, 135, 145, 255)

        local vw = draw.text_size(row[2])
        draw.text(x + PANEL_W - 14 - vw, ry, row[2], 255, 255, 255, 255)
    end
end)
```

## What to notice

* **Drawing lives in `"draw"`, not `"frame"`.** Calling `draw.*` from `"frame"`
  silently produces nothing.
* **`draw.text_size` right-aligns the values** instead of guessing pixel offsets,
  so it stays lined up whatever the numbers are.
* **`draw.screen_size()` on every frame**, not cached at load time — the window
  can be resized.
* `tab:get("corner")` returns a **1-based** index, matching the order the items
  were declared.
