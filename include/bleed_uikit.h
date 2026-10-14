#pragma once
#include <stdint.h>
#include <stddef.h>

// all of the kit in one header, hand it out with libuikit.a

// colours

// the windows 9x layout in bleeds colours, graphite where 9x was grey and blood where it was navy
#define UI_COLOUR_FACE         0x2B2B2F
#define UI_COLOUR_HILIGHT      0x5A5A61
#define UI_COLOUR_LIGHT        0x3D3D43
#define UI_COLOUR_SHADOW       0x17171A
#define UI_COLOUR_DKSHADOW     0x000000

#define UI_COLOUR_CAPTION_L    0x4A0404    // oxblood
#define UI_COLOUR_CAPTION_R    0xC1121F
#define UI_COLOUR_CAPTION_TEXT 0xFFFFFF
#define UI_COLOUR_BACK_L       0x26262A
#define UI_COLOUR_BACK_R       0x3A3A40
#define UI_COLOUR_BACK_TEXT    0x8C8C93

#define UI_COLOUR_BUTTON       0x38383E
#define UI_COLOUR_BUTTON_HI    0x64646C
#define UI_COLOUR_BUTTON_LO    0x121214
#define UI_COLOUR_BUTTON_IN_HI 0x47474E    // the second edge inside the first, a 9x button has two
#define UI_COLOUR_BUTTON_IN_LO 0x222226
#define UI_COLOUR_BUTTON_X     0xE8E8EA
#define UI_COLOUR_BUTTON_LIT   0x4C4C54    // minimise and maximise with the pointer on them
#define UI_COLOUR_HOT          0xD0182A    // the close button with the pointer on it
#define UI_COLOUR_HOT_HI       0xF0606C
#define UI_COLOUR_HOT_LO       0x6A0810
#define UI_COLOUR_HOT_IN_HI    0xE23A4A
#define UI_COLOUR_HOT_IN_LO    0x9A1220
#define UI_COLOUR_HOT_DOWN     0x8E0D17

#define UI_COLOUR_WARN         0xF5B301    // the memory warning in the tray

#define UI_COLOUR_TEXT         0xE8E8EA    // anything written straight onto the face colour

#define UI_COLOUR_INFO         0x3B78D8    // the info and question signs, the one thing that isnt red
#define UI_COLOUR_SIGN_DARK    0x17171A    // whats written on the warning sign

typedef struct {
    int x, y;
    int w, h;
} ui_rect_t;

// somewhere to draw. a windows pixels, or any other buffer of 0x00RRGGBB
typedef struct {
    uint32_t *pixels;
    int       width;
    int       height;
    int       pitch;    // pixels from one row to the next
    ui_rect_t clip;     // nothing gets drawn outside this
} ui_canvas_t;

ui_canvas_t ui_canvas(uint32_t *pixels, int width, int height, int pitch);

// keep the drawing inside r until its taken off again
void ui_clip(ui_canvas_t *c, ui_rect_t r);
void ui_unclip(ui_canvas_t *c);

ui_rect_t ui_rect_clip(ui_rect_t r, ui_rect_t bounds);
ui_rect_t ui_rect_shrink(ui_rect_t r, int by);
int       ui_rect_contains(ui_rect_t r, int x, int y);

// shapes

void ui_fill(ui_canvas_t *c, ui_rect_t r, uint32_t colour);

// one pixel of 3d edge round r, the two colours are what make it look raised or sunken
void ui_edge(ui_canvas_t *c, ui_rect_t r, uint32_t top_left, uint32_t bottom_right);

// from at the left (or top) of r fading to the other colour
void ui_gradient(ui_canvas_t *c, ui_rect_t r, uint32_t from, uint32_t to, int vertical);

// text

#define UI_FONT_FIRST   32
#define UI_FONT_GLYPHS  95  // printable ascii

typedef struct {
    uint8_t  w, h;
    int8_t   left, top;     // where the bitmap sits from the pen and the top of the line
    uint8_t  advance;
    uint32_t offset;        // into the fonts coverage
} ui_glyph_t;

typedef struct {
    int               line_height;
    int               ascent;
    const ui_glyph_t *glyphs;
    const uint8_t    *coverage;
} ui_font_t;

// the fonts live here, a file is picked by how its name ends: Barlow-SemiBold-ui.ttf
#define UI_FONT_DIR     "/initrd/fonts"

#define UI_FONT_REGULAR "-ui.ttf"
#define UI_FONT_BOLD    "-bold.ttf"
#define UI_FONT_LIGHT   "-thin.ttf"

extern ui_font_t ui_font_text;      // what nearly everything is written in
extern ui_font_t ui_font_heading;
extern ui_font_t ui_font_light;     // for whatever goes under a heading

// reads the three fonts above. call it once before drawing any text, -1 if theres no -ui.ttf
int ui_init(void);

// the font whose file ends in role, px tall. -1 if theres no such file or it cant be read,
// and out is left empty, text in it draws nothing
int ui_font_load(ui_font_t *out, const char *role, int px);
int ui_font_load_file(ui_font_t *out, const char *path, int px);

int ui_font_height(const ui_font_t *font);
int ui_text_width(const ui_font_t *font, const char *text);

// y is the top of the line, not the baseline
void ui_text(ui_canvas_t *c, const ui_font_t *font, int x, int y, const char *text, uint32_t colour);

enum {
    UI_ALIGN_LEFT,
    UI_ALIGN_CENTRE,
    UI_ALIGN_RIGHT,
};

// one line of text in the middle of r from top to bottom, and across it wherever align says
void ui_text_in(ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align);

// the same over as many lines as it takes: a \n starts a new one.
int  ui_text_wrap(ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align);

// how wide and tall ui_text_wrap would make it in a box max_w across, without drawing it
void ui_text_measure(const ui_font_t *font, int max_w, const char *text, int *w, int *h);

// images

// w * h pixels of 0xAARRGGBB
typedef struct {
    uint32_t *pixels;
    int       w, h;
} ui_image_t;

// a png file. 0 if it worked
int  ui_image_load(ui_image_t *img, const char *path);

// a file thats nothing but w * h pixels of b g r a, the boot splash is one
int  ui_image_load_raw(ui_image_t *img, const char *path, int w, int h);
void ui_image_free(ui_image_t *img);

// make it w by h. smaller is every pixel it covers averaged, so it stays smooth
int  ui_image_scale(ui_image_t *img, int w, int h);

// for art thats a glow on black: how bright a pixel is becomes how solid it is, so the black
// goes and whatevers behind shows through
void ui_image_key_black(ui_image_t *img);

void ui_image_draw(ui_canvas_t *c, const ui_image_t *img, int x, int y);

// elements

enum {
    UI_ICON_NONE,
    UI_ICON_INFO,
    UI_ICON_QUESTION,
    UI_ICON_WARNING,
    UI_ICON_ERROR,
};

#define UI_ICON_PX 32

// one of the signs above, UI_ICON_PX square with its top left at x y
void ui_icon(ui_canvas_t *c, int icon, int x, int y);

enum {
    UI_RAISED,
    UI_SUNKEN,
};

// a box in the face colour standing out of whats round it, or a darker one set into it
void ui_panel(ui_canvas_t *c, ui_rect_t r, int style);

// an etched groove w long, two pixels tall
void ui_separator(ui_canvas_t *c, int x, int y, int w);

// a bar thats permille thousandths full
void ui_progress(ui_canvas_t *c, ui_rect_t r, int permille);

typedef struct {
    ui_rect_t   rect;
    const char *label;

    // the kits own, leave these alone
    uint8_t     hover;
    uint8_t     held;
    uint8_t     was_left;
} ui_button_t;

#define UI_CHANGED  (1 << 0)    // it looks different now, draw it again
#define UI_CLICKED  (1 << 1)    // pressed and let go on the button

void ui_button_draw(ui_canvas_t *c, const ui_button_t *b);

// tell the button where the pointer is and whether the left button is down
int  ui_button_mouse(ui_button_t *b, int x, int y, int left);

// message boxes

// message boxes are windows, so a program that uses these links libcrime after the kit

#define UI_MSGBOX_BUTTONS   4

// what ui_msgbox gives back when no button was pressed
#define UI_MSGBOX_CLOSED    (-1)    // closed from the title bar, or escape
#define UI_MSGBOX_FAILED    (-2)    // no desktop, no window from it, or no font to write with

typedef struct {
    const char *title;
    const char *text;
    int         icon;                           // UI_ICON_*, none unless one is asked for

    // left to right
    const char *buttons[UI_MSGBOX_BUTTONS];

    int         default_button;
    int         escape_button;
    int         width;
    int         align;                          // UI_ALIGN_* for the text
} ui_msgbox_t;

// puts the box up and waits. which button was pressed, counting from 0 in the order given,
// or one of the two above
int ui_msgbox(const ui_msgbox_t *box);

// the two that come up most: something to read with an OK under it, and a yes or no thats
// 1 for yes and 0 for anything else
void ui_message(const char *title, const char *text, int icon);
int  ui_ask(const char *title, const char *text);
