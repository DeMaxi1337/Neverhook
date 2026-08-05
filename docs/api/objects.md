# Level objects

A script can read and manipulate the objects of the level that is currently
playing. **Creating objects is not possible on purpose** — the API is there to
read and change what the level already contains, not to build levels.

Every function below needs a level to be running; outside a level they return
`0`, `nil` or an empty table.

## Finding objects

| Function | Description |
| --- | --- |
| `gd.objects.count()` | How many objects the level has |
| `gd.objects.all()` | Every object, up to 4096 per call |
| `gd.objects.by_id(id)` | Objects with a given object id (e.g. `8` for a spike) |
| `gd.objects.by_group(group)` | Objects that belong to a group |
| `gd.objects.near(x, y, radius)` | Objects inside a circle, in level coordinates |
| `gd.objects.toggle_group(group, on)` | Triggers a group the way a toggle trigger would |

```lua
local spikes = gd.objects.by_id(8)
client.log(#spikes .. " spikes in this level")
```

## Object methods

```lua
local obj = gd.objects.all()[1]

local x, y = obj:position()
obj:set_position(x, y + 30)

obj:set_rotation(obj:rotation() + 45)
obj:set_scale(1.5)
obj:set_color(255, 40, 40)
obj:set_opacity(128)
obj:set_visible(false)
```

| Method | Returns |
| --- | --- |
| `position()` / `set_position(x, y)` | `x, y` |
| `rotation()` / `set_rotation(deg)` | degrees |
| `scale()` / `set_scale(value)` | number |
| `color()` / `set_color(r, g, b)` | `r, g, b` in `0..255` |
| `opacity()` / `set_opacity(value)` | `0..255` |
| `visible()` / `set_visible(flag)` | boolean |
| `id()` | object id |
| `type()` | object type as a number |
| `groups()` | table of group ids |
| `in_group(group)` | boolean |
| `is_decoration()` | boolean |

## Handles go stale

An object handle points into the level that was loaded when you got it. When
the level changes, the whole registry is cleared, and touching an old handle
raises:

```
this object handle is no longer valid (the level changed)
```

That is a normal Lua error you can catch with `pcall` — it will never crash the
game. Fetch objects again after `level_start`.

## Player and camera

| Function | Description |
| --- | --- |
| `gd.player_state([which])` | Table with position, velocity, gamemode, size, gravity, dead, on ground |
| `gd.set_player_velocity(y [, which])` | Sets the vertical velocity |
| `gd.set_player_rotation(deg [, which])` | Sets the rotation |
| `gd.set_player_scale(value [, which])` | Sets the scale |
| `gd.press(button, down [, which])` | `1` jump, `2` left, `3` right |
| `gd.camera()` | Camera position and zoom |
| `gd.set_camera(x, y)` | Moves the camera |
| `gd.set_zoom(value)` | Camera zoom |
| `gd.world_to_screen(x, y)` | Level coordinates to screen pixels |
| `gd.screen_to_world(x, y)` | Screen pixels to level coordinates |
| `gd.song_time()` | Position of the song in seconds |
| `gd.is_paused()` | Whether the level is paused |

`world_to_screen` is what you want when drawing on top of the level:

```lua
callbacks.add("draw", function()
    for _, obj in ipairs(gd.objects.by_id(8)) do
        local x, y = obj:position()
        local sx, sy = gd.world_to_screen(x, y)
        draw.circle(sx, sy, 6, 255, 60, 60)
    end
end)
```
