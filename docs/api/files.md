# fs

A private folder for your script's own data: `scripts/data`.

`fs.*` can never see anything outside it. No `..`, no absolute paths, no drive
letters, and only these extensions: `.txt`, `.json`, `.csv`, `.log`, `.ini`,
`.dat`, `.md`, `.cfg`. Writing `.lua` is refused on purpose -- a script must not
be able to write code that later runs.

For a couple of values, [storage](storage.md) is simpler. Use `fs` when you need
real files: exports, logs, caches, big tables.

```lua
fs.write("runs.csv", "level,percent\n")
fs.append("runs.csv", gd.level().name .. "," .. gd.level().percent .. "\n")

local text = fs.read("runs.csv")
if text then print(#str.split(text, "\n") - 1 .. " runs saved") end
```

## Functions

| Function | Returns |
| --- | --- |
| `fs.read(name)` | contents, or `nil, reason` |
| `fs.write(name, text)` | `true`, or `false, reason` |
| `fs.append(name, text)` | same |
| `fs.exists(name)` | boolean |
| `fs.size(name)` | bytes, or `nil` |
| `fs.remove(name)` | boolean |
| `fs.mkdir(folder)` | boolean |
| `fs.list([folder])` | array of file names |
| `fs.list_dirs([folder])` | array of folder names |
| `fs.folder()` | full path, for showing the user |
| `fs.open_folder()` | opens it in the file manager |

Subfolders are allowed: `fs.write("exports/2026.json", data)`. Parent folders are
created for you.

## Limits

* 4 MB per read or write.
* 512 entries per listing.
* Paths up to 160 characters.

# script

Information about the running script, and a way to split it across files.

| Function | |
| --- | --- |
| `script.import(name)` | loads `scripts/lib/<name>.lua` once and caches it |
| `script.name()` | the file name |
| `script.id()` | internal id |
| `script.folder()` | the scripts folder |
| `script.libs()` | names available to `import` |
| `script.log(text [, is_error])` | writes straight to the Script console |

`scripts/lib/vec3.lua`:

```lua
local M = {}

function M.dist(x1, y1, x2, y2)
    return vec.distance(x1, y1, x2, y2)
end

return M
```

Your script:

```lua
local vec3 = script.import("vec3")
print(vec3.dist(0, 0, 3, 4))   -- 5
```

A library sees exactly the same sandbox your script does -- same globals, same
limits. Only plain `.lua` source is accepted: precompiled bytecode is refused,
there are no search paths, and nothing can be imported from a string or from the
network.
