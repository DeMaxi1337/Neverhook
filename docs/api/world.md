# world

`gd.*` reads the level, `world.*` acts on it: reset, kill, pause, practice mode,
checkpoints, camera shake, counts.

Every function starts from the real game layer and does nothing at all when
there is no level, so it is always safe to call from any callback without
checking first.

```lua
callbacks.add("death", function()
    if world.is_practice() then
        world.reset()          -- straight back to the last checkpoint
    end
end)
```

## Where are we

| Function | Returns |
| --- | --- |
| `world.in_level()` | in a level right now |
| `world.in_editor()` | in the editor |
| `world.scene_name()` | `"play"`, `"editor"` or `"menu"` |
| `world.screen_size()` | width, height |

## Run control

| Function | |
| --- | --- |
| `world.reset()` | the normal restart, honours checkpoints |
| `world.reset_from_start()` | ignores the start position |
| `world.full_reset()` | full level reset |
| `world.kill()` | kills the player through the game's own path, so the death effect, attempt counter and every hook fire normally |
| `world.pause([unfocused])` | returns `false` if the game refused |
| `world.resume()` | |
| `world.restart([from_start])` | resume and restart in one step |
| `world.can_pause()` | |
| `world.is_paused()` | |
| `world.set_practice(on)` / `world.is_practice()` | |
| `world.process_checkpoints()` | |
| `world.toggle_ground(visible)` | |
| `world.toggle_practice_music()` | |
| `world.complete()` | ends the run as a completion; always logged to the console |

## State

| Function | Returns |
| --- | --- |
| `world.time()` | seconds since the attempt started |
| `world.progress()` | current progress |
| `world.object_count()` | objects in the level |
| `world.selected_count()` | selected objects, editor only |
| `world.has_start_pos()` | the run began at a start position |
| `world.ending()` | the end animation has started |
| `world.shake(on [, factor])` | camera shake, factor 0..10 |

## Player and settings

`world.player_name()` returns the local player name, or `nil`.

`world.game_variable(key)` reads one game setting by its key, e.g.
`world.game_variable("0026")`. Writing game variables is deliberately **not**
exposed: a script must never silently change the user's own settings.

## Example: a practice-run helper

```lua
local deaths = 0

callbacks.add("death", function()
    deaths = deaths + 1

    if deaths >= 5 and not world.is_practice() then
        world.set_practice(true)
        client.notify("five deaths -- practice mode on")
    end
end)

callbacks.add("level_start", function() deaths = 0 end)
```
