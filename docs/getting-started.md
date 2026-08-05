# Getting started

## Where scripts live

Scripts are read from the Neverhook save folder:

```
<Geode save dir>/demaxi1337.neverhook/scripts/
```

The quickest way there is **Scripts → Scripts folder** in the footer of the
Scripts tab. Only files ending in `.lua` are picked up, and only the top level
of the folder is scanned — no subfolders.

## Creating a script

Press **Create** in the top-right of the Scripts tab and type a name. That
writes a small starter file and reloads the list. You can also just drop a
`.lua` file into the folder and press the refresh icon.

## Loading and stopping

Every row has a **Load** button. Loading a script:

1. reads the file as text (precompiled bytecode is refused),
2. creates a fresh sandboxed VM for that script alone,
3. runs the file top to bottom,
4. keeps the VM alive so its callbacks can fire.

**Stop** closes the VM, drops every callback and removes any tab the script
added. Nothing is left behind, so Load again is a clean restart.

Scripts never start on their own. Nothing runs until you press Load.

## Errors

A script that raises an error is stopped and the message, with a traceback, is
printed to the console (the terminal icon in the top row) and shown in red
under the row. A callback that never returns is aborted by the watchdog after
100 ms with `script aborted: exceeded its time budget`.

## Structure of a typical script

```lua
-- 1. build the UI once, at load time
local tab = menu.tab("My script")
tab:toggle("enabled", "Enabled", false)
tab:slider("power", "Power", 0, 100, 50)

-- 2. restore saved settings
tab:set("enabled", storage.get("enabled", false))

-- 3. do the work in a callback
callbacks.add("frame", function()
    if not tab:get("enabled") then return end
    -- ...
end)

-- 4. persist on change
callbacks.add("frame", function()
    storage.set("enabled", tab:get("enabled"))
end)
```

Build UI at load time, not inside callbacks — calling `menu.tab` every frame
would add a new sidebar entry every frame.

## Sharing scripts

Share the `.lua` source, never a compiled file. Neverhook refuses LuaJIT
bytecode on purpose, so a `.lua` file someone hands you is always readable text
you can check before loading.
