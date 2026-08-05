# Timers and input

## Timers

Timers run on the game thread between frames, so a callback can safely touch
the game and the menu.

### `client.set_timer(seconds, fn) -> id`

Repeats `fn` forever until you stop it.

```lua
local id = client.set_timer(1.0, function()
    client.log("tick")
end)
```

### `client.delay_call(seconds, fn) -> id`

Runs `fn` once.

```lua
client.delay_call(0.5, function() gd.alert("Hi", "half a second later") end)
```

### `client.stop_timer(id)`

Cancels a timer. Every timer of a script is dropped when the script stops.

A script may keep up to **256** timers alive at a time.

## Frame information

| Function | Description |
| --- | --- |
| `client.frame_time()` | Seconds the last frame took |
| `client.fps()` | Frames per second |
| `client.menu_open()` | `true` while the Neverhook menu is visible |
| `gd.frame()` | Frames since the game started |

## Mouse and keyboard

| Function | Description |
| --- | --- |
| `client.key_down(code)` | Virtual-key state |
| `client.mouse_pos()` | Returns `x, y` in screen pixels |
| `client.mouse_down(button)` | `1` left, `2` right, `3` middle |
| `client.mouse_wheel()` | Wheel movement this frame |

```lua
callbacks.add("frame", function()
    local x, y = client.mouse_pos()
    if client.mouse_down(1) then
        client.log(string.format("click at %.0f %.0f", x, y))
    end
end)
```
