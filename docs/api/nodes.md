# node

The game's own UI. ImGui is for a menu-style HUD; `node.*` creates real cocos2d
nodes, so a script can put a label on the pause screen, a sprite next to the
player, or a Geometry Dash button anywhere the game itself could.

Every node is an integer **handle**, never a pointer. Handles stay valid even if
the game removes the node, and everything a script created is pulled out of the
scene automatically when the script stops.

```lua
local label = node.label("hello from Lua", "goldFont.fnt")
node.set_pos(label, 200, 200)
node.set_scale(label, 0.6)
node.add(nil, label)          -- nil parent = the running scene
node.fade_to(label, 0, 2)     -- fade out over two seconds
```

## Finding nodes

| Function | Returns |
| --- | --- |
| `node.scene()` | the running scene |
| `node.play_layer()` | `PlayLayer`, or `nil` outside a level |
| `node.editor()` | the editor layer, or `nil` |
| `node.scene_name()` | `"play"`, `"editor"`, `"menu"` or `"other"` |
| `node.find(parent_or_nil, id)` | child by Geode node id |
| `node.query(parent_or_nil, selector)` | Geode `querySelector` syntax |
| `node.children(handle)` | array of handles (max 256) |
| `node.child_count(handle)` / `node.child_at(handle, i)` | `i` is 1-based |
| `node.parent(handle)` | |
| `node.id(handle)` / `node.set_id(handle, id)` | |
| `node.count()` | how many handles are alive |

Node ids come from `geode.node-ids`, so `node.find(nil, "main-menu")` finds what
the game calls `main-menu`.

## Creating nodes

| Function | Creates |
| --- | --- |
| `node.label(text [, font])` | `CCLabelBMFont`, default `bigFont.fnt` |
| `node.sprite(name)` | sprite from the atlas, falling back to a file |
| `node.button_sprite(text [, texture, width])` | a `ButtonSprite` |
| `node.button(sprite_handle, fn)` | clickable item; add it to a `node.menu()` |
| `node.menu()` | `CCMenu` container, needed for buttons to react |
| `node.layer_color(r, g, b [, a, w, h])` | solid colour layer |
| `node.scale9(texture [, w, h])` | stretchable panel, e.g. `GJ_square01.png` |
| `node.empty()` | plain container node |

```lua
local menu   = node.menu()
local sprite = node.button_sprite("Click me")
local button = node.button(sprite, function()
    client.notify("clicked")
end)

node.add(menu, button)
node.set_pos(menu, 100, 100)
node.add(nil, menu)
```

Only plain resource names are accepted: no paths, no `..`, no drive letters.

## Tree

`node.add(parent_or_nil, child [, z])`, `node.remove(handle)`,
`node.remove_children(handle)`, `node.keep_across_scenes(handle)`,
`node.forget(handle)` (drops the handle without touching the node, useful when
walking a large tree).

## Properties

`node.pos` / `set_pos` / `move` / `world_pos`, `node.size` / `set_size`,
`node.scale` / `set_scale`, `node.rotation` / `set_rotation`,
`node.set_anchor`, `node.zorder` / `set_zorder`,
`node.visible` / `set_visible`, `node.opacity` / `set_opacity`,
`node.color` / `set_color` (0..255), `node.text` / `set_text`.

`set_text` works on labels and on `ButtonSprite`; anything else raises an error.
Colour and opacity silently do nothing on nodes that have no colour.

## Actions

| Function | |
| --- | --- |
| `node.move_to(h, x, y, seconds [, ease])` | |
| `node.move_by(h, dx, dy, seconds [, ease])` | |
| `node.scale_to(h, scale, seconds [, ease])` | |
| `node.rotate_to(h, degrees, seconds [, ease])` | |
| `node.rotate_by(h, degrees, seconds [, ease])` | |
| `node.fade_to(h, opacity, seconds)` | 0..255 |
| `node.tint_to(h, r, g, b, seconds)` | |
| `node.pulse(h [, scale, seconds])` | endless breathing loop |
| `node.delay_then(h, seconds, fn)` | timer tied to the node |
| `node.stop_actions(h)` | |

Easing names: `linear`, `in`, `out`, `in_out`, `sine_in`, `sine_out`,
`elastic`, `bounce`, `back`.

## Limits

* 512 live handles at a time across all scripts; call `node.forget()` when you
  are done with one.
* Up to 256 children per `node.children()` call.
* Text is capped at 512 characters.

## Player, world layer, shadows, smoothing

| Function | Description |
| --- | --- |
| `node.player()` | the first player's node, or nil outside a level |
| `node.player2()` | the second player's node |
| `node.object_layer()` | the layer level objects live in: add nodes here to place them in the world |
| `node.shadow(handle, dx, dy, opacity, spread)` | black copy of a sprite's texture as a child at z = -1, follows the parent automatically |
| `node.smooth_to(handle, x, y, factor)` | move a fraction of the remaining distance, called every frame |
| `node.smooth_rotate(handle, angle, factor)` | same for rotation, along the shortest arc |
| `node.smooth_scale(handle, scale, factor)` | same for scale |
| `node.set_flip(handle, flipX, flipY)` | flip a sprite |

`factor` is clamped to 0..1 and defaults to `0.15`. `node.shadow` only works on
sprites and returns a handle for the shadow, so you can fade or recolour it
later. Defaults are `dx = 3`, `dy = -3`, `opacity = 120`, `spread = 1.05`.

See `examples/companion.md` for a companion that levitates next to the player
with a shadow.

## Where a custom node may live

The game keeps most level graphics inside sprite batches. A batch only accepts
its own sprites: pushing anything else into it corrupts the batch and takes the
game down, so `node.add()` refuses a batch parent with an error instead.

`node.object_layer()` handles that for you. It returns the level's object layer
when that layer is a normal node, and otherwise a container of ours that sits
next to the batch and copies its position, scale and rotation every frame. Either
way your node scrolls with the level and lands in world coordinates.

Handles to nodes the game owns (`node.player()`, `node.scene()`,
`node.object_layer()`, anything from `node.find()`) are dropped automatically when
the scene changes, so a stale handle gives you a clean Lua error instead of a
crash. Take a fresh handle after every level start. Nodes you created yourself
stay yours until `node.forget()`.

## node.valid

`node.valid(handle)` returns `true` while the handle still points at a live node.

Handles to the game's own nodes (`node.player()`, `node.object_layer()`, `node.find()`) are dropped when the scene changes, so check them before use instead of relying on an error:

```lua
if not (pet and node.valid(pet)) then
    pet = nil
    build()
end
```

## Argument order of node.add

`node.add(parent, child)` - the parent comes first. Passing them the other way round used to move the game's own layer into your sprite; that is now rejected with an error.

Only nodes your script created can be moved with `node.add`. Nodes that belong to the game can be read and animated, but not reparented.

## What node.object_layer returns

`node.object_layer()` returns a container node owned by the mod (`nh-lua-world`) that is attached to the game layer and follows the object layer's position, scale and rotation every frame. Add your world-space nodes to it. It is recreated on every scene change, so take a fresh handle after the level starts.
