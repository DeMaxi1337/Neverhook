# cls - game classes

`cls` reaches game objects by name: fields you can read and write, methods you can
call. Every class, field and method is on a whitelist that is checked at compile
time, so a typo gives you a Lua error instead of a crash.

```lua
print(cls.get("PlayLayer", "m_isPaused"))
cls.set("PlayerObject", "m_yVelocity", 12)
cls.call("PlayLayer", "togglePracticeMode", true)
```

## Functions

| Function | Description |
| --- | --- |
| `cls.list()` | array of class names |
| `cls.exists(class)` | is the instance alive right now |
| `cls.fields(class)` | array of `{ name, type, writable }` |
| `cls.methods(class)` | array of `{ name, args }` |
| `cls.get(class, field)` | field value, or nil when the instance is gone |
| `cls.set(class, field, value)` | true when it was written |
| `cls.call(class, method, ...)` | calls the method, returns its result if it has one |

Every class resolves to the live instance on its own: `PlayLayer` is the current
play layer, `PlayerObject` is player one, `PlayerObject2` is player two,
`GJGameState` is the state block of the current game layer. Outside a level they
resolve to nothing, `cls.get` returns nil and `cls.set` returns false, so a
script that runs in the menu never crashes.

## Classes

### PlayLayer

Fields: `m_isPaused`, `m_pauseDelta`, `m_isPracticeMode`,
`m_levelEndAnimationStarted` (read only).

Methods: `resetLevel`, `resetLevelFromStart`, `fullReset`, `delayedResetLevel`,
`delayedFullReset`, `resume`, `processCheckpoints`, `levelComplete`, `onQuit`,
`pauseGame(bool)`, `resumeAndRestart(bool)`, `togglePracticeMode(bool)`,
`toggleGroundVisibility(bool)`, `canPauseGame()`.

### GJBaseGameLayer

Fields: `m_clickBetweenSteps`, `m_clickOnSteps`.

Methods: `resetLevelVariables`, `handleButton(down, button, player1)`,
`processQueuedButtons(dt, clearQueue)`, `shakeCamera(duration, strength, interval)`.

### GJGameState

Fields: `m_currentProgress`, `m_levelTime`, `m_timeWarp`, `m_isDualMode`,
`m_cameraShakeEnabled`, `m_cameraShakeFactor`.

### PlayerObject and PlayerObject2

Fields: `m_yVelocity`, `m_fallSpeed`, `m_gravity`, `m_gravityMod`,
`m_speedMultiplier`, `m_vehicleSize`, `m_isDead`, `m_isOnGround`,
`m_isUpsideDown`, `m_isOnSlope`, `m_wasOnSlope`, `m_slopeVelocity`,
`m_slopeAngle`, `m_slopeStartTime`, `m_isCollidingWithSlope`,
`m_yVelocityBeforeSlope`, `m_dashX`, `m_dashY`, `m_dashAngle`, `m_dashStartTime`,
`m_jumpBuffered`, `m_stateRingJump`, `m_wasTeleported`, `m_fixGravityBug`, and
the read only `m_lastCollisionTop`, `m_lastCollisionBottom`,
`m_lastCollisionLeft`, `m_lastCollisionRight`.

Methods: `pushButton(button)`, `releaseButton(button)`, `playDeathEffect`,
`incrementJumps`. Button 1 is jump, 2 is left, 3 is right.

### GameManager

Methods: `getGameVariable(key)`, `setGameVariable(key, value)`.

### LevelEditorLayer

No fields or methods yet, but `cls.exists("LevelEditorLayer")` tells you whether
the editor is open.

## Why the whitelist

Raw memory access from a script would mean any `.lua` file could execute
anything, which is exactly what the mod is not allowed to ship. Fields are
reached through typed accessors generated from the 2.2081 bindings, so a wrong
type is a compile error on our side and a plain Lua error on yours.

Missing something? Ask for the class and the field, adding them is a two line
change.
