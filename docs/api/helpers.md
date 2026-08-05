# str, tbl, col, vec, ease

The boring functions every script rewrites. They are pure Lua-side helpers: no
game state, no side effects, safe to call from anywhere.

## str

| Function | |
| --- | --- |
| `str.split(text, separator)` | array of pieces |
| `str.join(array [, separator])` | |
| `str.trim(text)` | |
| `str.starts_with(text, prefix)` / `str.ends_with(text, suffix)` | |
| `str.contains(text, needle)` | |
| `str.replace(text, from, to)` | plain text, not a pattern |
| `str.lower(text)` / `str.upper(text)` | |
| `str.pad(text, width [, char, right])` | |
| `str.number(value [, decimals])` | `1234567` -> `"1 234 567"` |
| `str.time(seconds [, with_ms])` | `83.5` -> `"1:23.50"` |

```lua
for _, word in ipairs(str.split("a,b,c", ",")) do print(word) end
print(str.time(gd.frame_time()))
```

## tbl

`tbl.copy(t [, deep])`, `tbl.count(t)`, `tbl.keys(t)`, `tbl.values(t)`,
`tbl.index_of(t, value)`, `tbl.contains(t, value)`, `tbl.reverse(t)`,
`tbl.slice(t, first [, last])`, `tbl.merge(a, b)`.

## col

Colours are 0..255 everywhere.

| Function | |
| --- | --- |
| `col.hsv(h, s, v [, a])` | h in degrees -> r, g, b, a |
| `col.to_hsv(r, g, b)` | -> h, s, v |
| `col.hex("#3b82f6")` | -> r, g, b, a |
| `col.to_hex(r, g, b)` | -> `"#3b82f6"` |
| `col.lerp(r1, g1, b1, r2, g2, b2, t)` | blend two colours |
| `col.rainbow([speed, offset])` | animated colour, uses the clock |

```lua
local r, g, b = col.rainbow(1.5)
draw.text("rainbow", 40, 40, r, g, b)
```

## vec

`vec.add`, `vec.sub`, `vec.scale`, `vec.length`, `vec.distance`,
`vec.normalize`, `vec.dot`, `vec.angle`, `vec.rotate` -- all take and return
plain `x, y` numbers, so nothing has to be wrapped in a table.

```lua
local px, py = gd.player_state().x, gd.player_state().y
local dist   = vec.distance(px, py, 500, 200)
```

## ease

`ease(name, t)` maps `t` in 0..1 through a curve. Names: `linear`,
`in_quad`, `out_quad`, `in_out_quad`, `in_cubic`, `out_cubic`, `in_out_cubic`,
`in_quart`, `out_quart`, `in_sine`, `out_sine`, `in_out_sine`, `in_expo`,
`out_expo`, `in_back`, `out_back`, `out_elastic`, `out_bounce`.

```lua
local alpha = ease("out_cubic", progress) * 255
```

## utils extras

These are added to the existing `utils` table:

| Function | |
| --- | --- |
| `utils.sign(x)`, `utils.map(x, a1, b1, a2, b2)` | |
| `utils.approach(current, target, step)` | frame-rate friendly movement |
| `utils.random([min, max])` | |
| `utils.hash(text)` | FNV-1a, hex string |
| `utils.base64_encode(text)` / `utils.base64_decode(text)` | |
| `utils.date([format])` | local time, `strftime` style |

See also [utils](utils.md) for the functions that were there before
(`clamp`, `lerp`, `round`, `clock`, ...).
