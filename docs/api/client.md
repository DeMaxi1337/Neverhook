# client

General runtime bits: logging, timing and key state.

## client.log(text)

Writes a line to the script console (the terminal icon in the Scripts tab) and
to the Geode log, prefixed with the script's file name.

```lua
client.log("loaded")
```

`print(...)` does the same thing and accepts several arguments, joining them
with tabs.

## client.error(text)

Same, but the line is red in the console and logged as a warning. Use it for
problems the user should notice. It does **not** stop the script.

```lua
if not gd.in_level() then
    client.error("start a level first")
end
```

## client.frame_time()

Seconds since the previous frame. Multiply by it to keep movement
framerate-independent.

```lua
local speed = 200 * client.frame_time()
```

## client.fps()

Current frames per second, smoothed.

```lua
draw.text(10, 10, string.format("%.0f fps", client.fps()))
```

## client.key_down(vk)

`true` while the key is held. `vk` is a Windows virtual-key code — the same
numbers the keybind system uses. `65` is `A`, `32` is space, `112`–`123` are
F1–F12.

```lua
callbacks.add("frame", function()
    if client.key_down(114) then   -- F3
        mod.set("Noclip", true)
    end
end)
```

On platforms other than Windows this always returns `false`.

It reports the raw key state, so it is `true` on every frame the key is held.
Track the previous state yourself if you want an edge:

```lua
local was = false
callbacks.add("frame", function()
    local now = client.key_down(114)
    if now and not was then
        print("F3 pressed")
    end
    was = now
end)
```

## client.menu_open()

`true` while the Neverhook menu is visible. Useful for hiding your own overlay
while the menu is up.
