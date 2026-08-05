# callbacks

Subscribing is how a script does anything after it finishes loading. The file
itself runs once; everything ongoing lives in a callback.

## callbacks.add(event, fn)

Registers `fn` for `event`. Returns the index it was stored at.

```lua
callbacks.add("frame", function()
    print("once per frame")
end)
```

Callbacks take no arguments and return nothing. If you need the arguments of the
game function itself, or want to cancel it, use [`hooks`](hooks.md) instead. Raising an error inside one
stops the whole script — otherwise it would spam the console every frame.

Each callback gets a **100 ms** budget. Overrun aborts the script.

### Events

| Event | Fires |
| --- | --- |
| `"frame"` | Once per rendered frame. Your main loop. |
| `"draw"` | Once per frame, right after `frame`. The only place [`draw`](draw.md) works. |
| `"level_start"` | A level finished loading. |
| `"level_end"` | A level was completed. |
| `"death"` | The player died. |
| `"reset"` | The level restarted (death respawn, R, checkpoint). |

An unknown event name is an error at load time, so a typo shows up immediately
instead of silently doing nothing.

You can add several callbacks to the same event; they run in registration order.

## callbacks.remove(event, index)

Drops one callback, using the index `callbacks.add` returned. Indices handed out
earlier stay valid.

```lua
local id = callbacks.add("frame", tick)
callbacks.remove("frame", id)
```

## callbacks.clear(event)

Drops every callback registered for `event`.

```lua
callbacks.clear("draw")
```

You rarely need this — stopping the script clears everything anyway.

## Notes

`frame` and `draw` fire whether the menu is open or closed, and whether or not
you are in a level. Guard with [`gd.in_level()`](gd.md) when your code needs a
level:

```lua
callbacks.add("frame", function()
    if not gd.in_level() then return end
    -- ...
end)
```
