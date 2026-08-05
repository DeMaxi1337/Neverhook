# hooks

[`callbacks`](callbacks.md) tells you *that* something happened. `hooks` puts you
**inside** the game function while it happens: you get its arguments, and on some
hooks you can stop it from running at all.

```lua
hooks.add("PlayLayer::destroyPlayer", function(e)
    if e.percent > 90 then
        return false   -- no death after 90%
    end
end)
```

## hooks.add(name, fn)

Subscribes `fn` to the hook called `name` and returns the index it was stored at
(pass it to `hooks.remove`). An unknown name is an error at load time, exactly
like `callbacks.add`, so a typo shows up immediately.

The callback receives **one table** with the hook's fields (see the table below).
Up to **32** callbacks per hook per script.

## hooks.remove(name, index)

Drops one subscription, using the index `hooks.add` returned. Other indices stay
valid.

## hooks.clear(name)

Drops every subscription for `name`. `hooks.clear()` with no argument drops all
of them.

## hooks.list()

Returns an array describing every hook this build exposes:

```lua
for _, h in ipairs(hooks.list()) do
    print(h.name, h.cancelable, h.count)
end
```

`count` is how many callbacks *this script* currently has on that hook. Use
`hooks.list()` for feature detection instead of hardcoding names: newer
Neverhook versions add hooks, older ones do not have them.

## Cancelling

Returning `false` from a callback skips the original game function — but only for
hooks marked cancelable. Returning `false` from anything else (`PlayLayer::init`,
`MenuLayer::init`, …) is ignored and logged once: cancelling those would just
crash the game, and a Lua script is not allowed to do that.

Anything other than `false` (including `nil`, i.e. falling off the end of the
function) lets the original run.

If several scripts subscribe to the same cancelable hook and **any** of them
returns `false`, the original is skipped.

## Available hooks

| Name | Cancelable | Fields |
| --- | --- | --- |
| `PlayLayer::init` | no | `level_id`, `level_name`, `platformer` |
| `PlayLayer::resetLevel` | no | `percent`, `practice`, `level_id`, `level_name`, `attempts` |
| `PlayLayer::destroyPlayer` | **yes** | same, plus `player2`, `object_id` |
| `PlayLayer::levelComplete` | no | `percent`, `practice`, `level_id`, `level_name`, `attempts` |
| `PlayLayer::togglePracticeMode` | no | `practice` |
| `GJBaseGameLayer::update` | no | `dt` |
| `GJBaseGameLayer::handleButton` | **yes** | `down`, `button`, `player1` |
| `GJBaseGameLayer::resetLevelVariables` | no | — |
| `PlayerObject::pushButton` | **yes** | `button`, `player2` |
| `PlayerObject::releaseButton` | **yes** | `button`, `player2` |
| `PlayerObject::playDeathEffect` | **yes** | `player2` |
| `PlayerObject::playSpawnEffect` | **yes** | `player2` |
| `MenuLayer::init` | no | — |
| `PauseLayer::customSetup` | no | — |
| `EditorPauseLayer::customSetup` | no | — |
| `EndLevelLayer::customSetup` | no | — |
| `LevelInfoLayer::init` | no | `level_id`, `level_name`, `challenge` |
| `EditorUI::init` | no | — |

`object_id` is the object ID that killed the player, or `-1` when the death had
no object (`destroyPlayer` from a trigger or from another mod).

`button` is `1` jump, `2` left, `3` right.

### When it fires relative to the original

* Cancelable hooks run **before** the original, so `return false` can stop it.
* `PlayLayer::levelComplete` also runs before the original, so you can read the
  attempt while it is still the current one.
* Everything else runs **after** the original, so `init` hooks see a layer that
  is fully built.

## Budgets, and why they are not the callback budget

`GJBaseGameLayer::update` runs every frame, and `handleButton` can run thousands
of times a second. The flat 100 ms per call limit that [`callbacks`](callbacks.md)
uses would let a slow hook freeze the game while never technically overrunning.

So hooks are metered differently:

* one call may not run longer than **20 ms** (watchdog, same as any callback
  the whole script dies if it hangs),
* all calls of **one hook, in one script, in one frame** share an **8 ms**
  budget,
* a script that blows that budget **30 frames in a row** loses *that one
  subscription*. The console says so, the rest of the script keeps running, and
  reloading the script re-enables it.

Practical rule: a hook body should be a few lines. If you need real work per
frame, set a flag in the hook and do the work in a `"frame"` callback.

## Recursion

A hook cannot re-enter itself. If your `handleButton` hook calls
[`gd.press`](gd.md), the resulting `handleButton` will not call your hook again —
otherwise the first click would recurse until the stack died.

## What you do not get

No pointers and no addresses. A hook hands you plain numbers, booleans and
strings, the same as the rest of the API. The list above is the whole list: hooks
in Geode are compile-time constructs, so there is no `hooks.add(address)` and
there never will be. If you need a hook that is not there, ask for it — adding
one is a few lines in `src/lua/LuaHooks.cpp`.

## Example: no-death runs that still count attempts

```lua
local tab = menu.tab("Immortal")
tab:toggle("on", "Block deaths", false)

local blocked = 0

hooks.add("PlayLayer::destroyPlayer", function(e)
    if not tab:get("on") then return end
    blocked = blocked + 1
    client.log(("blocked death at %.1f%% (object %d)"):format(e.percent, e.object_id))
    return false
end)

callbacks.add("draw", function()
    draw.text(20, 20, "blocked: " .. blocked, 255, 80, 80, 255)
end)
```

## Example: input logger

```lua
hooks.add("GJBaseGameLayer::handleButton", function(e)
    if e.down then
        client.log(("p%d pressed %d"):format(e.player1 and 1 or 2, e.button))
    end
end)
```

## Full hook list

Hooks are named after the real method they wrap, so `hooks.list()` is the source
of truth. A hook costs nothing until a script subscribes to it: the game checks a
listener counter before it builds the event table.

### PlayLayer

`PlayLayer::init`, `PlayLayer::resetLevel`, `PlayLayer::resetLevelFromStart`,
`PlayLayer::fullReset`, `PlayLayer::destroyPlayer`, `PlayLayer::levelComplete`,
`PlayLayer::togglePracticeMode`, `PlayLayer::postUpdate`, `PlayLayer::pauseGame`,
`PlayLayer::onQuit`, `PlayLayer::showCompleteEffect`,
`PlayLayer::storeCheckpoint`, `PlayLayer::loadFromCheckpoint`,
`PlayLayer::removeCheckpoint`, `PlayLayer::playEndAnimationToPos`,
`PlayLayer::addObject`.

### GJBaseGameLayer

`GJBaseGameLayer::update`, `GJBaseGameLayer::handleButton`,
`GJBaseGameLayer::resetLevelVariables`,
`GJBaseGameLayer::processQueuedButtons`, `GJBaseGameLayer::playerTouchedRing`,
`GJBaseGameLayer::playerTouchedTrigger`, `GJBaseGameLayer::gameEventTriggered`,
`GJBaseGameLayer::shakeCamera`.

### PlayerObject

`PlayerObject::pushButton`, `PlayerObject::releaseButton`,
`PlayerObject::playDeathEffect`, `PlayerObject::playSpawnEffect`,
`PlayerObject::update`, `PlayerObject::incrementJumps`,
`PlayerObject::playSpiderDashEffect`, `PlayerObject::ringJump`.

### Editor and menus

`LevelEditorLayer::postUpdate`, `EditorUI::init`,
`EditorPauseLayer::customSetup`, `MenuLayer::init`, `PauseLayer::customSetup`,
`EndLevelLayer::customSetup`, `UILayer::init`, `LevelInfoLayer::init`,
`LevelInfoLayer::levelDownloadFinished`,
`LevelInfoLayer::onEnterTransitionDidFinish`, `CreatorLayer::init`,
`GameManager::init`.

### Engine and objects

`CCKeyboardDispatcher::dispatchKeyboardMSG`,
`AppDelegate::applicationWillEnterForeground`,
`FMODAudioEngine::fadeOutMusic`, `GameObject::playShineEffect`,
`EffectGameObject::triggerObject`, `HardStreak::addPoint`.

## Cancelling

Return `false` from a callback to stop the original method. `hooks.list()` marks
which hooks accept that; on the others a returned `false` is ignored and the
console warns you once.

Useful cancellable ones: `destroyPlayer` (godmode), `playDeathEffect`,
`shakeCamera`, `playerTouchedRing`, `gameEventTriggered`, `fadeOutMusic`,
`playShineEffect`, `addPoint` (wave trail), `dispatchKeyboardMSG` (swallow a key).

```lua
hooks.add("GJBaseGameLayer::shakeCamera", function(e)
    return e.strength < 2
end)

hooks.add("CCKeyboardDispatcher::dispatchKeyboardMSG", function(e)
    if e.key == 84 and e.down then
        print("T was swallowed")
        return false
    end
end)
```

Every event table carries what the method received: `dt`, `percent`, `x`, `y`,
`player`, `object_id`, `key`, `down`, `duration`, `strength` and so on. Print the
table to see what a specific hook gives you. Each hook has its own time budget
per script: burn 8 ms a frame for 30 frames and that one hook is switched off
with a console line, the rest keep running.
