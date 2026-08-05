# A styled control panel

Shows every new widget in one tab: groups, a text field, a colour picker,
change callbacks, saved settings and coloured text.

```lua
local tab = menu.tab("Panel")

tab:label("NEVERHOOK", { color = { 90, 160, 255 }, gradient = { 255, 90, 200 },
                         animated = true, speed = 1.2 })

tab:group("General")
    tab:toggle("hud", "Show HUD", true)
    tab:slider("scale", "HUD scale", 0.5, 2.0, 1.0)
    tab:input_text("title", "HUD title", storage.get("title", "neverhook"))
    tab:color_picker("color", "HUD colour", 90, 160, 255, 255)
tab:group_end()

tab:group("Sound")
    tab:button("Test sound", function()
        audio.play_sfx("explode_11.ogg")
    end)
tab:group_end()

tab:text_style("hud", { color = { 140, 255, 180 } })

-- Remember the title between sessions.
tab:on_change("title", function(value)
    storage.set("title", value)
end)

tab:on_change("color", function()
    client.notify("colour updated", "success", 1.0)
end)

callbacks.add("draw", function()
    if not tab:get("hud") then return end

    local r, g, b = tab:get("color")
    local title   = tab:get("title")

    draw.rect_filled(16, 16, 220, 34, 10, 14, 24, 200, 6)
    draw.text(26, 26, title, r, g, b)
end)
```

## Notes

- `tab:group()` / `tab:group_end()` only affect layout; forgetting the closing
  call cannot break the menu.
- `on_change` fires while the user is typing or dragging, so keep the callback
  cheap.
- `storage` holds booleans, numbers and strings; use `json.encode` for tables.
