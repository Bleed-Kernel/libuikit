# Bleed UIKit

A drawing and widget library for programs on the Bleed desktop, so they all look the same and nobody has to copy the desktops drawing code again.

It draws into any buffer of pixels, a window from Crime-Scene or anything else. only the message boxes need the desktop.

## Features

- Fills, 3D edges and gradients
- TrueType text with wrapping and alignment
- PNG images with scaling
- Info, question, warning and error icons
- Panels, separators and progress bars
- Buttons
- Message boxes
- Bleeds colours in one header

## Using it

A program needs two files, `bin/libuikit.a` and `include/bleed_uikit.h`. Link `libuikit.a` before `libcrime.a`.

```c
#include <crime/crime.h>
#include <bleed_uikit.h>

ui_init();

cs_window_t *win = cs_window_open(300, 120, "Hello");
ui_canvas_t  c   = ui_canvas(win->pixels, win->width, win->height, win->pitch);

ui_fill(&c, c.clip, UI_COLOUR_FACE);
ui_text(&c, &ui_font_text, 12, 12, "Hello from Bleed", UI_COLOUR_TEXT);
cs_window_commit(win, 0, 0, win->width, win->height);
```

Call `ui_init` once before any text is drawn, it returns -1 when theres no font to read. colours are 0x00RRGGBB everywhere.

## Functions

### Canvas

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_canvas | uint32_t *pixels, int width, int height, int pitch | ui_canvas_t | Somewhere to draw |
| ui_clip | ui_canvas_t *c, ui_rect_t r | N/A | Keep the drawing inside r |
| ui_unclip | ui_canvas_t *c | N/A | Take the clip off again |
| ui_rect_clip | ui_rect_t r, ui_rect_t bounds | ui_rect_t | The part of r thats inside bounds |
| ui_rect_shrink | ui_rect_t r, int by | ui_rect_t | r pulled in on every side |
| ui_rect_contains | ui_rect_t r, int x, int y | 1 if its inside | Whether x y is in r |

### Shapes

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_fill | ui_canvas_t *c, ui_rect_t r, uint32_t colour | N/A | A solid rectangle |
| ui_edge | ui_canvas_t *c, ui_rect_t r, uint32_t top_left, uint32_t bottom_right | N/A | One pixel of 3D edge round r, the two colours make it raised or sunken |
| ui_gradient | ui_canvas_t *c, ui_rect_t r, uint32_t from, uint32_t to, int vertical | N/A | Fades from one colour to the other, left to right or top to bottom |

### Text

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_init | N/A | 0, or -1 if theres no `-ui.ttf` | Reads the three fonts below |
| ui_font_load | ui_font_t *out, const char *role, int px | 0, or -1 if it couldnt | The font in `/initrd/fonts` whose file ends in role |
| ui_font_load_file | ui_font_t *out, const char *path, int px | 0, or -1 if it couldnt | A font from any path |
| ui_font_height | const ui_font_t *font | int | Height of a line |
| ui_text_width | const ui_font_t *font, const char *text | int | How wide the text would be |
| ui_text | ui_canvas_t *c, const ui_font_t *font, int x, int y, const char *text, uint32_t colour | N/A | One line of text, y is the top of the line and not the baseline |
| ui_text_in | ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align | N/A | One line in the middle of r from top to bottom |
| ui_text_wrap | ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align | How tall it came out | As many lines as it takes, starting at the top of r |
| ui_text_measure | const ui_font_t *font, int max_w, const char *text, int *w, int *h | N/A | How big `ui_text_wrap` would make it, without drawing |

`ui_init` gives you three fonts:

| Name | File ends in | Description |
|---|---|---|
| ui_font_text | `-ui.ttf` | What nearly everything is written in |
| ui_font_heading | `-bold.ttf` | Headings |
| ui_font_light | `-thin.ttf` | Whatever goes under a heading |

The fonts are only ever read from `/initrd/fonts`, nothing is built in. With no `-bold.ttf` or `-thin.ttf` its the `-ui.ttf` at that size, and a font that fails to load is left empty so text in it draws nothing.

Align is `UI_ALIGN_LEFT`, `UI_ALIGN_CENTRE` or `UI_ALIGN_RIGHT`.

### Images

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_image_load | ui_image_t *img, const char *path | 0 if it worked | Load a PNG |
| ui_image_load_raw | ui_image_t *img, const char *path, int w, int h | 0 if it worked | A file thats nothing but w * h pixels of b g r a |
| ui_image_free | ui_image_t *img | N/A | Free the pixels |
| ui_image_scale | ui_image_t *img, int w, int h | 0 if it worked | Make it w by h |
| ui_image_key_black | ui_image_t *img | N/A | For art thats a glow on black, how bright a pixel is becomes how solid it is |
| ui_image_draw | ui_canvas_t *c, const ui_image_t *img, int x, int y | N/A | Draw it with its top left at x y |

### Elements

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_icon | ui_canvas_t *c, int icon, int x, int y | N/A | A `UI_ICON_*` sign, 32 pixels square |
| ui_panel | ui_canvas_t *c, ui_rect_t r, int style | N/A | A box thats `UI_RAISED` or `UI_SUNKEN` |
| ui_separator | ui_canvas_t *c, int x, int y, int w | N/A | An etched groove w long |
| ui_progress | ui_canvas_t *c, ui_rect_t r, int permille | N/A | A bar thats permille thousandths full |
| ui_button_draw | ui_canvas_t *c, const ui_button_t *b | N/A | Draw the button how it is right now |
| ui_button_mouse | ui_button_t *b, int x, int y, int left | `UI_CHANGED`, `UI_CLICKED` | Tell the button where the pointer is and whether left is down |

Icons are `UI_ICON_NONE`, `UI_ICON_INFO`, `UI_ICON_QUESTION`, `UI_ICON_WARNING` and `UI_ICON_ERROR`.

Call `ui_button_mouse` on every `CS_EV_MOUSE`. `UI_CHANGED` means it looks different and wants drawing again, `UI_CLICKED` means it was pressed and let go on the button.

### Message boxes

These open a window of their own, so they need the desktop.

| Function | Args | Return value | Description |
|---|---|---|---|
| ui_message | const char *title, const char *text, int icon | N/A | Something to read with an OK under it |
| ui_ask | const char *title, const char *text | 1 for yes, 0 for anything else | A yes or no |
| ui_msgbox | const ui_msgbox_t *box | The button pressed, counting from 0 | Put the box up and wait |

`ui_msgbox` gives `UI_MSGBOX_CLOSED` (-1) if it was closed without a button and `UI_MSGBOX_FAILED` (-2) if theres no desktop. the programs other windows hear nothing until its answered.

## Types

### ui_rect_t

| Type | Name | Description |
|---|---|---|
| int | x, y | Top left |
| int | w, h | Size |

### ui_canvas_t

| Type | Name | Description |
|---|---|---|
| uint32_t* | pixels | The buffer, 0x00RRGGBB |
| int | width | Width in pixels |
| int | height | Height in pixels |
| int | pitch | Pixels from one row to the next |
| ui_rect_t | clip | Nothing gets drawn outside this |

### ui_image_t

| Type | Name | Description |
|---|---|---|
| uint32_t* | pixels | w * h pixels of 0xAARRGGBB |
| int | w, h | Size |

### ui_button_t

| Type | Name | Description |
|---|---|---|
| ui_rect_t | rect | Where it is on the canvas |
| const char* | label | What it says |
| uint8_t | hover, held, was_left | The kits own, leave these alone |

### ui_msgbox_t

| Type | Name | Default value | Description |
|---|---|---|---|
| const char* | title | N/A | Title of the window |
| const char* | text | N/A | \n for a new line, a long line is wrapped |
| int | icon | `UI_ICON_NONE` | Which sign to show |
| const char*[4] | buttons | A lone OK | Left to right, the list stops at the first NULL |
| int | default_button | 0 | The one enter presses |
| int | escape_button | 0 | One more than the button escape and the close box count as, 0 and they give `UI_MSGBOX_CLOSED` |
| int | width | 0 | Of the window, 0 to fit the text |
| int | align | `UI_ALIGN_LEFT` | For the text |

## Colours

Every colour is in `bleed_uikit.h` as `UI_COLOUR_*`. its the windows 9x layout in bleeds colours, graphite where 9x was grey and blood where it was navy.

## Building

```sh
make BLIBC_DIR=../blibc
```

This gives you `bin/libuikit.a` and `bin/uikit-demo`, the demo shows most of whats in here.

blibc and Crime-Scene are cloned into `external/` when theres no checkout handed in, and Crime-Scene is pulled on every build. its `libcrime.a` and `crime/crime.h` are copied into `libcrime/`, which is all the build looks at. Pass `CRIME_DIR=../Crime-Scene` to build against your own checkout, that one is never pulled.

`make distclean` takes `bin`, `sysroot`, `external` and `libcrime` with it.
