# A floating companion

A sprite downloaded from the web that levitates next to the player with its
nickname above it, and both cast a shadow so they never blend into the
background.

The finished script ships in the archive as `scripts/soufiw.lua`, so you can drop
it straight into your scripts folder.

```lua
local IMAGE_URL = "https://soufiw.nl/soufiw.png"
local NICKNAME  = "Soufiw"

local OFFSET_X = -34
local OFFSET_Y = 30
local SCALE    = 0.55

local image, pet, tag, tagShadow
local asked = false
local time  = 0

local function build()
    local layer = node.object_layer()
    if not layer then return false end

    local path = draw.image_path(image)
    if not path then return false end

    pet = node.sprite_file(path)
    node.set_scale(pet, SCALE)
    node.set_zorder(pet, 600)
    node.add(pet, layer)

    node.shadow(pet, 5, -5, 120, 1.05)

    tag = node.label(NICKNAME, "bigFont.fnt")
    node.set_scale(tag, 0.4)
    node.set_zorder(tag, 601)
    node.add(tag, layer)

    tagShadow = node.label(NICKNAME, "bigFont.fnt")
    node.set_scale(tagShadow, 0.4)
    node.set_color(tagShadow, 0, 0, 0)
    node.set_opacity(tagShadow, 140)
    node.set_zorder(tagShadow, 600)
    node.add(tagShadow, layer)

    return true
end

callbacks.add("frame", function()
    if not gd.in_level() then return end

    if not asked then
        asked = true
        image = draw.image_url(IMAGE_URL)
    end

    if not pet then
        local ready = draw.image_ready(image)
        if not ready or not build() then return end
    end

    time = time + gd.frame_time()

    local px, py = gd.player_pos()
    if not px then return end

    local bob  = math.sin(time * 2.2) * 7
    local sway = math.cos(time * 1.4) * 4
    local tilt = math.sin(time * 1.8) * 7

    node.smooth_to(pet, px + OFFSET_X + sway, py + OFFSET_Y + bob, 0.11)
    node.smooth_rotate(pet, tilt, 0.08)
    node.smooth_scale(pet, SCALE + math.sin(time * 3) * 0.02, 0.1)

    local hx, hy = node.pos(pet)
    node.set_pos(tag, hx, hy + 22)
    node.set_pos(tagShadow, hx + 1.5, hy + 20.5)
end)
```

## Loading the image

`draw.image_url(url)` downloads on a worker thread and hands back a handle right
away. `draw.image_ready(handle)` returns `ready, failed`, and once it is ready
`draw.image_path(handle)` gives the cached file, which `node.sprite_file(path)`
turns into a real sprite in the level. Files are cached in
`scripts/images/cache`, so the download happens once and every later run is
instant. `node.sprite_file` only accepts paths inside that folder.

## Why it looks smooth

* `node.smooth_to` moves a fraction of the remaining distance every frame, so the
  companion lags behind the player and catches up. That lag is what reads as
  levitation. `0.11` is soft, `0.4` is snappy, `1.0` is an instant snap.
* `node.smooth_rotate` takes the shortest arc, so the tilt never spins the long
  way around when the angle crosses 180 degrees.
* Two sine waves at different speeds keep the idle motion from looking like a
  metronome, and a third one breathes the scale a little.
* `gd.frame_time()` instead of a fixed `1 / 60` keeps the motion identical on any
  refresh rate or speedhack.

## Shadows

`node.shadow(handle, dx, dy, opacity, spread)` adds a black copy of the sprite's
texture as a child at `z = -1`, so it follows every move, rotation and scale of
the parent with no per-frame work.

Labels are not sprites, so the nickname gets its shadow the manual way: a second
black label one pixel and a half below the first one.

The player is not a plain sprite either. Give it a shadow with its own dark
sprite following `gd.player_pos()` and `gd.player_rotation()`:

```lua
local glow = node.sprite("player_ball_01_001.png")
node.set_color(glow, 0, 0, 0)
node.set_opacity(glow, 110)
node.add(glow, node.object_layer())

callbacks.add("frame", function()
    local px, py = gd.player_pos()
    if not px then return end
    node.set_pos(glow, px + 4, py - 4)
    node.set_rotation(glow, gd.player_rotation())
end)
```

## Player tracking

| Function | Returns |
| --- | --- |
| `gd.player_pos([n])` | `x, y` |
| `gd.player_rotation([n])` | degrees |
| `gd.player_velocity([n])` | vertical velocity |
| `gd.player_size([n])` | `width, height` of the hitbox |
| `gd.player_state([n])` | a table with everything above plus flags |
| `gd.is_dual()` | is the level in dual mode |
| `node.player()`, `node.player2()` | the player's own node |
| `node.object_layer()` | the layer level objects live in |

Pass `2` to any of them for the second player. Nodes added to
`node.object_layer()` live in the world and scroll with the level; add them to
`node.scene()` instead to pin them to the screen.
