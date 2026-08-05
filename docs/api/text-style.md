# Text style

Text in your tab can be a flat colour, a gradient between two colours, or an
animated rainbow. Colours are always given as `0..255` components.

## Style table

| Key | Type | Meaning |
| --- | --- | --- |
| `color` | `{r, g, b[, a]}` | Base colour |
| `gradient` | `{r, g, b[, a]}` | Second colour; turns the text into a gradient |
| `animated` | `boolean` | Slides the gradient along the text |
| `rainbow` | `boolean` | Cycles through the whole hue circle |
| `speed` | `number` | Animation speed, clamped to `0.05 .. 20` |

## tab:label(text \[, style])

```lua
local tab = menu.tab("Style")

tab:label("Plain text")
tab:label("Warning", { color = { 255, 80, 80 } })
tab:label("Fade",    { color = { 80, 160, 255 }, gradient = { 255, 80, 220 } })
tab:label("Moving",  { color = { 80, 160, 255 }, gradient = { 255, 80, 220 },
                       animated = true, speed = 1.5 })
tab:label("Rainbow", { rainbow = true, speed = 2 })
```

## tab:text\_style(id, style)

Applies a style to any widget that already exists. A flat colour tints the
widget's own text, so it works on toggles, sliders and buttons too. Gradients
and rainbows only apply to labels and group headers.

```lua
tab:toggle("godmode", "God mode", false)
tab:text_style("godmode", { color = { 120, 255, 140 } })
```

## Drawing styled text on screen

The same effects are available in the `draw` table, for overlays drawn during
the `draw` event.

```lua
callbacks.add("draw", function()
    draw.text_gradient(20, 40, "Neverhook", 80, 160, 255, 255, 80, 220)
    draw.text_rainbow(20, 60, "Speedrun mode", 1.5)
end)
```

- `draw.text_gradient(x, y, text, r1, g1, b1, r2, g2, b2 [, a])`
- `draw.text_rainbow(x, y, text [, speed, alpha])`

## Grouping

A styled group header keeps long tabs readable:

```lua
tab:group("Visuals")
tab:toggle("trail", "Trail", false)
tab:toggle("glow", "Glow", false)
tab:group_end()
```
