# gd

Reads the running level and the player. Everything here returns plain Lua
values — a script never sees a pointer or an address.

All of it is safe to call outside a level; you get `false` or `nil` instead of a
crash.

## gd.in_level()

`true` while a level is running.

```lua
if not gd.in_level() then return end
```

## gd.level()

A table describing the current level, or `nil` if there is none.

| Field | Type | |
| --- | --- | --- |
| `name` | string | Level name |
| `id` | number | Level id |
| `attempts` | number | Total attempts |
| `best` | number | Best normal-mode percent |
| `percent` | number | Current percent |
| `platformer` | boolean | Platformer level |
| `practice` | boolean | Practice mode is on |

```lua
local lvl = gd.level()
if lvl then
    print(lvl.name .. " " .. lvl.percent .. "%")
end
```

## gd.player(index)

A table describing a player. `index` is `1` or `2` and defaults to `1`. Returns
`nil` outside a level.

| Field | Type | |
| --- | --- | --- |
| `x` | number | X position |
| `y` | number | Y position |
| `rotation` | number | Rotation in degrees |
| `y_velocity` | number | Vertical velocity |
| `dead` | boolean | Currently dead |
| `on_ground` | boolean | Touching the ground |
| `upside_down` | boolean | Gravity is flipped |

```lua
local p = gd.player()
if p and p.y_velocity > 0 then
    print("going up")
end
```

The table is a snapshot. Writing to it does nothing; use `gd.set_player_pos`.

## gd.set_player_pos(x, y, index)

Moves a player. `index` is `1` or `2` and defaults to `1`.

```lua
local p = gd.player()
if p then
    gd.set_player_pos(p.x, p.y + 30)
end
```

This is the same kind of teleport the menu's own features do — it goes through
the game's node position, not through memory.

## gd.time()

Seconds elapsed in the current attempt, `0` outside a level.

```lua
print(string.format("%.2fs", gd.time()))
```
