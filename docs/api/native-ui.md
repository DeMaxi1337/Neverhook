# Native UI

Besides its own tab, a script can open real Geometry Dash popups. They look
exactly like the game's own dialogs and work while the menu is closed.

Every popup is asynchronous: it returns immediately and calls your function
later, once the user clicks. If the script is stopped in the meantime, the
callback is simply dropped.

## gd.alert(title, text)

A message with a single OK button.

```lua
gd.alert("Neverhook", "Practice mode is on.")
```

## gd.confirm(title, text, callback)

Yes/no dialog. The callback receives `1` for Yes and `0` for No.

```lua
gd.confirm("Reset", "Clear saved stats?", function(answer)
    if answer == 1 then storage.set("stats", "") end
end)
```

## gd.text\_input(title, callback)

Opens a text field. The callback receives the text as a string.

```lua
gd.text_input("Preset name", function(name)
    client.log("saving " .. name)
end)
```

## gd.dropdown(title, items, callback)

A stack of buttons, up to ten. The callback receives the 1-based index of the
chosen item.

```lua
gd.dropdown("Mode", { "Off", "Safe", "Aggressive" }, function(index)
    client.log("picked " .. index)
end)
```

## gd.color\_picker(r, g, b, callback)

Opens the game's colour picker. The callback runs every time the colour changes
and receives `r, g, b` in the `0..255` range.

```lua
gd.color_picker(255, 80, 80, function(r, g, b)
    tab:set("tint", r, g, b, 255)
end)
```

The picker is only available when the build of Geode you are running exposes
it; otherwise the call raises a normal Lua error you can catch with `pcall`.

## client.notify(text)

A small toast in the corner. Good for feedback that does not deserve a dialog.

```lua
client.notify("Layout mode enabled")
```

## Mixing with the menu

Native popups and your ImGui tab are independent — use whichever fits. A common
pattern is a button in your tab that opens a popup:

```lua
local tab = menu.tab("Tools")

tab:button("Rename preset", function()
    gd.text_input("New name", function(name)
        if name ~= "" then storage.set("preset", name) end
    end)
end)
```
