# Sandbox and security

Scripts get shared around. A scripting system that can open files, load DLLs or
call arbitrary addresses is a malware delivery system, so Neverhook's runtime is
built so that a hostile `.lua` file simply has nothing to reach for.

## One VM per script

Each loaded script gets its own `lua_State`. Scripts cannot see or modify each
other's globals, and stopping one cannot disturb another.

## What is removed from the standard library

After the base library is opened, these globals are deleted:

`dofile`, `loadfile`, `load`, `loadstring`, `require`, `module`, `package`,
`io`, `debug`, `newproxy`, `gcinfo`, `setfenv`, `getfenv`

`os` is rebuilt with only `clock`, `time`, `date` and `difftime` — so no
`os.execute`, `os.remove`, `os.rename`, `os.getenv`, `os.tmpname`.

The libraries that stay are `string`, `table`, `math` and `bit`.

## No FFI

`ffi` is never opened. This is the single most important line in the runtime.
`ffi.cdef` plus `ffi.C` lets a script declare and call *any* exported function
at *any* address — `WinExec`, `LoadLibraryA`, `VirtualProtect`. That is native
code execution, i.e. exactly the remote-code-execution hole this design exists
to close.

## No raw memory access

There is deliberately no `memory.read/write` table either. Being able to write
an arbitrary byte to an arbitrary address is the same primitive as calling an
arbitrary function: overwrite a vtable entry or a return address and you have
native execution again. Scripts get typed accessors like `gd.player()` instead,
never an address.

## No bytecode

LuaJIT does not verify bytecode. A malformed `.luac` chunk can corrupt VM memory
and escape the sandbox. Neverhook rejects any chunk starting with the `0x1B`
Lua signature byte and compiles with text-only mode, so only readable source can
run.

## No filesystem, no arbitrary network

Scripts never receive a path. `storage.get`/`storage.set` write into Neverhook's
own save file under a `script.<name>.` prefix, so a script cannot read another
script's data, let alone anything outside the mod.

There is no HTTP function. The one thing that touches the network is
`draw.image_url`, which fetches a URL, refuses anything that is not a PNG/JPG,
caps the size, and hands back a texture handle — never bytes and never a string.
So a script cannot download a payload and run it: there is no way to smuggle in
code that the person who shared the script never saw. Do keep in mind that a URL
is still a request, so a script *can* tell a server that you ran it.

## Watchdog

A LuaJIT count hook fires every 20 000 VM instructions and checks the clock. A
callback gets 100 ms, the initial run of the file gets 2 s. Going over raises a
normal Lua error, which unwinds into the surrounding `lua_pcall`:

* the offending script stops and reports the error,
* every other script keeps running,
* the game does not freeze.

So `while true do end` costs one script, not your session.

## Hook budgets

[`hooks`](api/hooks.md) run inside game functions, some of them thousands of times
a second, so the flat per-callback limit is not enough there: one call gets
20 ms, and all calls of one hook in one script in one frame share 8 ms. A script
that overruns 30 frames in a row loses that single subscription and keeps
running. Cancelling is limited to a whitelist of hooks where skipping the
original is survivable — a script cannot cancel `PlayLayer::init` and take the
game down with it.

Hooks are still just C++ hooks written in `src/lua/LuaHooks.cpp`. Lua can only
subscribe to what that file already declares; there is no "hook this address".

## Error handling

Everything crossing into Lua goes through `lua_pcall` with a traceback handler.
No C++ exceptions are thrown across the boundary.

## What a script can still do

It can toggle anything the menu can toggle, move the player, and draw on the
screen. In other words a script is as powerful as a person clicking around the
menu very quickly — and no more powerful than that.
