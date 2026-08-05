# imgui

Full ImGui windows from a script. `draw.*` paints on top of the screen; `imgui.*`
builds real windows with widgets the user can interact with.

{% hint style="warning" %}
`imgui.*` only works inside a `"draw"` callback. Outside of it every function is
a no-op, because there is no frame to draw into.
{% endhint %}

```lua
local opacity = 100
local enabled = false

callbacks.add("draw", function()
    if imgui.begin_window("My panel", "auto_resize") then
        enabled = imgui.checkbox("Enabled", enabled)
        opacity = imgui.slider("Opacity", opacity, 0, 255)

        if imgui.button("Reset") then
            opacity = 100
        end
    end
    imgui.end_window()
end)
```

## The two rules

1. **Every `begin_*` needs its `end_*`**, even when it returned `false`
   (`begin_window`, `begin_child`, `begin_group`, `begin_tab_bar`,
   `begin_tab_item`, `begin_table`, `begin_combo`, `tree_node`, popups).
2. **Widget state lives in your script.** A widget takes the current value and
   returns the new one:

```lua
value = imgui.slider("Speed", value, 0, 10)
```

If a script errors out with windows still open, the menu unwinds them itself at
the end of the frame, so a bug never breaks the rest of the UI.

## Windows and layout

| Function | Notes |
| --- | --- |
| `imgui.begin_window(title, ...flags)` | `true` when visible; always pair with `end_window()` |
| `imgui.end_window()` | |
| `imgui.begin_child(id, w, h [, border])` / `end_child()` | scrollable sub-area |
| `imgui.begin_group()` / `end_group()` | groups items for layout |
| `imgui.set_next_pos(x, y [, pivot_x, pivot_y])` | position for the next window |
| `imgui.set_next_size(w, h)` | size for the next window |
| `imgui.set_next_bg_alpha(a)` | 0..1 |
| `imgui.window_pos()` / `window_size()` / `content_avail()` | two numbers each |
| `imgui.cursor_pos()` / `set_cursor_pos(x, y)` / `cursor_screen_pos()` | |
| `imgui.same_line([offset])`, `spacing()`, `new_line()`, `separator()` | |
| `imgui.indent([w])` / `unindent([w])` / `dummy(w, h)` | |
| `imgui.set_next_width(w)`, `push_item_width(w)` / `pop_item_width()` | |
| `imgui.push_id(id)` / `pop_id()` | for identical labels in a loop |
| `imgui.begin_disabled([disabled])` / `end_disabled()` | greys out widgets |
| `imgui.frame_height()`, `font_size()` | |

Window flags are strings, passed as extra arguments or as one array:
`no_title`, `no_resize`, `no_move`, `no_scrollbar`, `no_scroll_with_mouse`,
`no_collapse`, `auto_resize`, `no_background`, `no_saved_settings`,
`no_mouse_inputs`, `menu_bar`, `horizontal_scroll`, `no_focus_on_appear`,
`no_bring_to_front`, `vertical_scrollbar`, `no_nav`, `no_decoration`,
`no_inputs`.

## Text

`imgui.text(s)`, `text_colored(s, r, g, b [, a])`, `text_disabled(s)`,
`text_wrapped(s)`, `bullet_text(s)`, `label_text(label, value)`,
`text_size(s)` (returns width, height).

## Widgets

| Function | Returns |
| --- | --- |
| `imgui.button(label [, w, h])` | `true` on click |
| `imgui.small_button(label)` | `true` on click |
| `imgui.invisible_button(id, w, h)` | `true` on click |
| `imgui.checkbox(label, value)` | new value, changed |
| `imgui.radio(label, active)` | `true` on click |
| `imgui.slider(label, value, min, max [, format])` | new value, changed |
| `imgui.slider_int(label, value, min, max)` | new value, changed |
| `imgui.drag(label, value [, speed, min, max])` | new value, changed |
| `imgui.drag_int(label, value [, speed, min, max])` | new value, changed |
| `imgui.input_text(label, text [, max_len])` | new text, changed |
| `imgui.input_multiline(label, text [, w, h])` | new text, changed |
| `imgui.input_number(label, value [, step])` | new value, changed |
| `imgui.combo(label, index, items)` | new index (1-based), changed |
| `imgui.listbox(label, index, items [, visible_rows])` | new index, changed |
| `imgui.color_edit(label, r, g, b [, a])` | r, g, b, a, changed |
| `imgui.progress_bar(fraction [, overlay])` | |
| `imgui.selectable(label [, selected])` | `true` on click |
| `imgui.collapsing_header(label)` | `true` when open |
| `imgui.tree_node(label)` + `tree_pop()` | `true` when open |

Colours are always 0..255, and list indices are always 1-based, like everywhere
else in this API.

## Tabs, tables, popups, tooltips

```lua
if imgui.begin_tab_bar("tabs") then
    if imgui.begin_tab_item("First") then
        imgui.text("hello")
        imgui.end_tab_item()
    end
    imgui.end_tab_bar()
end

if imgui.begin_table("stats", 2, "row_bg", "borders") then
    imgui.table_column("Key")
    imgui.table_column("Value")
    imgui.table_headers()

    imgui.table_next_row()
    imgui.table_next_column(); imgui.text("FPS")
    imgui.table_next_column(); imgui.text(tostring(gd.fps()))

    imgui.end_table()
end
```

Table flags: `resizable`, `reorderable`, `hideable`, `sortable`, `row_bg`,
`borders`, `borders_inner`, `borders_outer`, `scroll_x`, `scroll_y`, `stretch`,
`fixed`, `no_clip`. Columns can also be sized with
`imgui.table_set_column(index)`.

Popups: `imgui.open_popup(id)`, `begin_popup(id)`, `begin_popup_modal(title)`,
`end_popup()`, `close_popup()`. Tooltips: `imgui.set_tooltip(text)`.

Plots: `imgui.plot_lines(label, values [, w, h])`,
`imgui.plot_histogram(label, values [, w, h])`.

## Style

```lua
imgui.push_style_color("button", 40, 90, 255)
imgui.push_style_var("frame_rounding", 6)
imgui.button("Rounded and blue")
imgui.pop_style_var()
imgui.pop_style_color()
```

Colour keys include `text`, `window_bg`, `child_bg`, `popup_bg`, `border`,
`frame_bg` (+ `_hovered`, `_active`), `title_bg` (+ `_active`), `button`
(+ `_hovered`, `_active`), `header` (+ `_hovered`, `_active`), `check_mark`,
`slider_grab`, `separator`, `plot_lines`, `table_header_bg`, `text_selected_bg`.
`imgui.style_color(key)` reads one back.

Style vars include `alpha`, `window_padding` (two numbers), `window_rounding`,
`frame_padding` (two numbers), `frame_rounding`, `item_spacing` (two numbers),
`indent`, `scrollbar_size`, `grab_rounding`, `button_align` (two numbers).

## Queries

`imgui.is_item_hovered()`, `is_item_clicked([button])`, `is_item_active()`,
`is_window_hovered()`, `is_window_focused()`.

## Extra drawing

The same file also extends `draw.*` with
`circle_filled`, `rect_rounded`, `rect_rounded_filled`, `rect_gradient`,
`triangle`, `triangle_filled`, `quad`, `quad_filled`, `ngon`, `ngon_filled`,
`bezier`, `polyline`, `poly_filled`, `text_scaled`, `push_clip` / `pop_clip`.
See [draw](draw.md).
