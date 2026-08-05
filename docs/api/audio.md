# audio

Plays sounds from your own files or from a link. Every URL a script opens is
printed to the Script console, so you can always see what a script is loading.

Local files live in `Geode/save/demaxi1337.neverhook/scripts/sounds/`. Only
plain file names are accepted: no folders, no `..`, no drive letters. Allowed
extensions are `.mp3`, `.ogg`, `.wav` and `.flac`.

## audio.play\_local(file)

Plays a file from the `sounds` folder and returns a handle, or `nil` when the
file could not be opened.

```lua
local id = audio.play_local("hit.mp3")
```

## audio.play\_url(url)

Streams audio from an `http://` or `https://` link. The sound starts as soon as
enough data has arrived, so the game never freezes while it loads. Returns a
handle.

```lua
local id = audio.play_url("https://example.com/track.mp3")
```

## audio.play\_sfx(name)

Plays one of Geometry Dash's own sound effects by file name.

```lua
audio.play_sfx("explode_11.ogg")
```

## audio.stop(handle)

Stops one sound.

## audio.stop\_all()

Stops every sound this script started.

## audio.set\_volume(handle, volume)

Volume goes from `0.0` to `1.0`.

## audio.is\_playing(handle)

Returns `true` while the sound is still loading or playing.

## Limits

- 32 sounds per script at a time.
- Finished sounds are released automatically, so you do not have to clean up
  after short effects.
- Stopping a script stops and frees all of its sounds.

```lua
local tab = menu.tab("Audio")
local beep

tab:button("Play", function()
    beep = audio.play_local("beep.wav")
    audio.set_volume(beep, 0.5)
end)

tab:button("Stop", function()
    audio.stop(beep)
end)
```
