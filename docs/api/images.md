# Images

A script can draw a PNG or JPG on screen — either a file it ships, or one it
downloads once and reuses.

Local files live in:

```
<Geode save dir>/mods/demaxi1337.neverhook/scripts/images/
```

Only plain file names are accepted, and only `.png`, `.jpg`, `.jpeg`.

## Loading

### `draw.image_local(name) -> handle`

Loads a file from the `images` folder. The texture is cached, so calling it
twice is cheap.

### `draw.image_url(url) -> handle`

Starts a background download and returns a handle straight away. Nothing
blocks: the picture appears once it arrived. The file is cached on disk, so the
next launch loads it locally.

The URL is printed to the Script console, downloads time out after 20 seconds
and anything over 8 MB is refused.

### `draw.image_ready(handle) -> boolean`

`true` once the image can be drawn.

### `draw.image_size(handle) -> width, height`

Pixel size, or `0, 0` while the image is still loading.

### `draw.image_free(handle)`

Releases the texture. Everything is released automatically when the script
stops.

## Drawing

### `draw.image(handle, x, y [, width, height, alpha])`

Valid inside the `draw` callback. Without a size the image is drawn at its
native resolution.

```lua
local logo

callbacks.add("draw", function()
    logo = logo or draw.image_url("https://example.com/logo.png")

    if draw.image_ready(logo) then
        draw.image(logo, 20, 20, 128, 128, 200)
    end
end)
```

## Limits

| Limit | Value |
| --- | --- |
| Images alive per script | 32 |
| Download size | 8 MB |
| Download timeout | 20 s |
