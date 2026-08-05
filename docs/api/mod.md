# mod

Flips Neverhook's own features by name. A script reaches exactly the features
that are registered with the bind system — the same set the keybind menu shows,
no more.

Names are the labels used in the menu, for example `"Noclip"`, `"Speedhack"`,
`"Show Hitboxes"`, `"Safe Mode"`. Use `mod.list()` to see what your build has.

## mod.list()

An array of every feature name.

```lua
for _, name in ipairs(mod.list()) do
    print(name)
end
```

## mod.get(name)

The on/off state of a feature. Errors on an unknown name.

```lua
if mod.get("Noclip") then
    print("noclip is on")
end
```

## mod.set(name, value)

Turns a feature on or off.

```lua
mod.set("Noclip", true)
mod.set("Show Hitboxes", false)
```

The change is exactly what clicking the checkbox does, so it also shows up in
the keybind overlay and is saved with the config.

## mod.get_value(name)

The numeric value attached to a feature, for the ones that have a slider.
Errors if the feature has no value.

```lua
print(mod.get_value("Speedhack"))
```

## mod.set_value(name, value)

Writes that value. It is clamped to the range the feature registered.

```lua
mod.set("Speedhack", true)
mod.set_value("Speedhack", 1.5)
```

## Notes

A feature name that does not exist raises an error rather than failing quietly,
so a renamed feature shows up in the console instead of turning your script into
a no-op. Guard optional features:

```lua
local ok = pcall(mod.set, "Some Feature", true)
if not ok then
    print("this build has no Some Feature")
end
```
