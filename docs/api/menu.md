# menu

Builds your script's own tab. A script that calls `menu.tab` makes a **Scripts**
group appear in the left sidebar, with one entry per tab.

Call this at load time, never inside a callback.

## menu.tab(title)

Creates a tab and returns a handle.

```lua
local tab = menu.tab("Bhop")
```

The tab and everything in it disappear when the script is stopped.

---

Every method below returns the tab handle, so calls can be chained:

```lua
menu.tab("Demo")
    :toggle("a", "Alpha", false)
    :slider("b", "Beta", 0, 10, 5)
```

Widget ids must be unique inside a tab; a duplicate is an error.

## tab:toggle(id, label, default)

A checkbox.

```lua
tab:toggle("enabled", "Enabled", false)
```

| Argument | Type | |
| --- | --- | --- |
| `id` | string | key used with `get`/`set` |
| `label` | string | shown in the menu |
| `default` | boolean | initial state |

## tab:slider(id, label, min, max, default)

A float slider. `default` is optional and falls back to `min`.

```lua
tab:slider("speed", "Speed", 0.5, 3.0, 1.0)
```

## tab:slider_int(id, label, min, max, default)

Same, with whole numbers.

```lua
tab:slider_int("count", "Count", 1, 10, 3)
```

## tab:combo(id, label, items, default)

A dropdown. `items` is an array of strings, `default` is a **1-based** index and
defaults to `1`.

```lua
tab:combo("mode", "Mode", { "Off", "Normal", "Aggressive" }, 2)
```

`tab:get` returns the selected index, 1-based.

## tab:button(label, fn)

A button that calls `fn` when clicked. Buttons have no id and no value.

```lua
tab:button("Reset", function()
    print("reset pressed")
end)
```

The callback gets the same 100 ms budget as any other callback.

## tab:label(text)

A line of static text.

```lua
tab:label("Works in platformer levels only.")
```

## tab:separator()

A horizontal rule.

## tab:get(id)

Current value of a widget: boolean for a toggle, number for a slider, 1-based
index for a combo.

```lua
if tab:get("enabled") then ... end
```

An unknown id is an error.

## tab:set(id, value)

Writes a value from the script side. Slider values are clamped to their range,
and an out-of-range combo index is ignored.

```lua
tab:set("speed", 2.0)
```

Handy for restoring saved settings at load time:

```lua
tab:set("enabled", storage.get("enabled", false))
```
