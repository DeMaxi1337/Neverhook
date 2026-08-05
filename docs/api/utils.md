# utils

Three maths helpers that come up constantly. Everything else you need is in the
standard `math`, `string`, `table` and `bit` libraries, which are all available.

## utils.clamp(value, min, max)

Keeps `value` inside the range.

```lua
local x = utils.clamp(input, 0, 100)
```

## utils.lerp(a, b, t)

Linear interpolation. `t` of `0` gives `a`, `1` gives `b`.

```lua
-- smooth a value towards a target, framerate-independent enough for UI work
current = utils.lerp(current, target, 10 * client.frame_time())
```

`t` is not clamped, so values outside 0–1 extrapolate.

## utils.round(value)

Rounds to the nearest whole number (halves go up).

```lua
print(utils.round(2.5))   -- 3
print(utils.round(2.4))   -- 2
```

## What else is available

| Library | Notes |
| --- | --- |
| `math` | Complete |
| `string` | Complete, including `string.format` |
| `table` | Complete |
| `bit` | LuaJIT's bit operations |
| `os` | Only `clock`, `time`, `date`, `difftime` |

See [Sandbox and security](../security.md) for why the rest is missing.
