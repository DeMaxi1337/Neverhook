# Object scanner

Highlights every hazard near the player and counts them, with a toggle that can
be bound to a key like any built-in feature.

```lua
local tab = menu.tab("Scanner")

tab:label("Hazard scanner", { color = { 255, 90, 90 }, gradient = { 255, 200, 90 } })
tab:bindable("on", "Enabled", false)
tab:slider("radius", "Radius", 50, 600, 260)
tab:color_picker("tint", "Marker colour", 255, 70, 70, 255)

tab:on_change("on", function()
    client.notify(tab:get("on") and "Scanner on" or "Scanner off")
end)

callbacks.add("draw", function()
    if not tab:get("on") or not gd.in_level() then return end

    local player = gd.player(1)
    if not player then return end

    local radius = tab:get("radius")
    local r, g, b = tab:get("tint")

    local found = gd.objects.near(player.x, player.y, radius)
    local count = 0

    for _, obj in ipairs(found) do
        if not obj:is_decoration() then
            local x, y   = obj:position()
            local sx, sy = gd.world_to_screen(x, y)

            draw.circle(sx, sy, 8, r, g, b)
            count = count + 1
        end
    end

    draw.text_gradient(20, 120, "objects near you: " .. count,
                       r, g, b, 255, 255, 255)
end)
```

Things worth copying from this script:

- `tab:bindable` puts the toggle into **All Hotkeys** under `Scripts/Scanner`.
- `tab:get("tint")` returns four values, so `local r, g, b` just takes the
  first three.
- Everything is drawn inside the `draw` callback — `draw.*` is not allowed
  anywhere else.
