# draw

Draws on top of the game. Only call these from a `"draw"` callback — anywhere
else the frame has already been submitted and nothing appears.

Coordinates are screen pixels, origin at the top-left corner.

Colours are four optional numbers `r, g, b, a` in the range 0–255. Leave them
out for opaque white.

```lua
callbacks.add("draw", function()
    draw.text(20, 20, "hello", 255, 80, 80, 255)
end)
```

## draw.screen_size()

Returns width and height, as two values.

```lua
local w, h = draw.screen_size()
draw.text(w / 2, h - 40, "centred-ish")
```

## draw.text(x, y, text, r, g, b, a)

Draws a string with its top-left corner at `x, y`.

## draw.text_size(text)

Width and height the string would take, as two values. Use it to centre things.

```lua
local tw, th = draw.text_size("centred")
local w = draw.screen_size()
draw.text(w / 2 - tw / 2, 30, "centred")
```

## draw.rect(x, y, w, h, r, g, b, a)

An outlined rectangle.

```lua
draw.rect(10, 10, 120, 40, 0, 150, 255, 255)
```

## draw.rect_filled(x, y, w, h, r, g, b, a, rounding)

A filled rectangle. `rounding` is an optional corner radius, default `0`.

```lua
draw.rect_filled(10, 10, 120, 40, 0, 0, 0, 160, 6)
```

## draw.line(x1, y1, x2, y2, r, g, b, a, thickness)

A line. `thickness` defaults to `1`.

```lua
draw.line(0, 100, 400, 100, 255, 255, 255, 60, 2)
```

## draw.circle(x, y, radius, r, g, b, a, thickness)

An outlined circle centred on `x, y`. `thickness` defaults to `1`.

```lua
draw.circle(200, 200, 25, 255, 255, 0, 255, 2)
```

## A panel, put together

```lua
callbacks.add("draw", function()
    if not gd.in_level() then return end

    draw.rect_filled(20, 20, 160, 44, 10, 12, 18, 200, 6)
    draw.rect(20, 20, 160, 44, 255, 255, 255, 20)
    draw.text(32, 32, string.format("%.1f%%", gd.level().percent), 90, 160, 255, 255)
end)
```
