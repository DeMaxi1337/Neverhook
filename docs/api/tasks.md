# task

Callbacks force everything into a state machine. A task is a coroutine the menu
resumes for you, so a script can simply say what happens in order.

```lua
task.run(function()
    client.notify("three")
    task.wait(1)
    client.notify("two")
    task.wait(1)
    client.notify("one")
    client.notify("go")
end)
```

## Functions

| Function | |
| --- | --- |
| `task.run(fn [, delay])` | starts a task, returns its id |
| `task.wait([seconds])` | pauses the task; `task.wait()` = next frame |
| `task.wait_frames([count])` | pauses for N game frames |
| `task.cancel(id)` | stops one task |
| `task.cancel_all()` | stops all of this script's tasks |
| `task.running()` | how many are alive |
| `task.alive(id)` | |
| `task.is_task()` | `true` inside a task body |

`task.wait` and `task.wait_frames` only work **inside** a task -- calling them
from a normal callback raises a clear error.

## Waiting for a condition

There is no `task.wait_until`: a plain loop is shorter and does the same thing.

```lua
task.run(function()
    while not world.in_level() do task.wait() end
    client.notify("level started")
end)
```

## Where tasks run

Tasks resume on the main thread, once per frame, after timers. Each resume gets
its own time budget (100 ms) with its own instruction hook, so a task that spins
without waiting is aborted with
`task aborted: it ran too long without waiting (infinite loop?)`
instead of freezing the game.

A task belongs to its script: stopping or reloading the script drops every task
it started.

## Limits

* 64 tasks per script.
* `task.wait` accepts up to one hour.
* `task.wait_frames` accepts up to 100000 frames.

## Example: an intro sequence

```lua
local panel = nil

callbacks.add("level_start", function()
    task.run(function()
        panel = node.label(gd.level().name, "bigFont.fnt")
        node.set_pos(panel, 300, 400)
        node.set_scale(panel, 0.1)
        node.add(nil, panel)

        node.scale_to(panel, 0.8, 0.4, "out_back")
        task.wait(2)

        node.fade_to(panel, 0, 0.6)
        task.wait(0.6)

        node.remove(panel)
    end)
end)
```
