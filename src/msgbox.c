#include <string.h>
#include <crime/crime.h>
#include <bleed_uikit.h>

#define PAD             16
#define ICON_GAP        14      // icon to the text
#define TEXT_MAX_W      340     // where a line gets wrapped when no width was asked for
#define MIN_W           220

#define BUTTON_H        24
#define BUTTON_MIN_W    76
#define BUTTON_PAD      14      // either side of a buttons label
#define BUTTON_GAP      8
#define BUTTONS_TOP     18      // between the text and the buttons

// set 1 scancodes, thats what a key event carries
#define KEY_ESCAPE      0x01
#define KEY_ENTER       0x1C
#define KEY_IS_DOWN     1

typedef struct {
    const ui_msgbox_t *box;
    ui_button_t        buttons[UI_MSGBOX_BUTTONS];
    int                count;
    int                buttons_w;   // all of them and the gaps between
    int                icon_w;      // the icon and its gap, 0 without one
    int                text_w;
    int                text_h;
    int                body_h;      // the taller of the icon and the text
} msgbox_t;

static void layout(msgbox_t *m, const ui_msgbox_t *box) {
    memset(m, 0, sizeof(*m));
    m->box = box;

    while (m->count < UI_MSGBOX_BUTTONS && box->buttons[m->count]) {
        m->buttons[m->count].label = box->buttons[m->count];
        m->count++;
    }
    if (!m->count)
        m->buttons[m->count++].label = "OK";

    for (int i = 0; i < m->count; i++) {
        int w = ui_text_width(&ui_font_text, m->buttons[i].label) + BUTTON_PAD * 2;
        m->buttons[i].rect.w = w < BUTTON_MIN_W ? BUTTON_MIN_W : w;
        m->buttons[i].rect.h = BUTTON_H;
        m->buttons_w += m->buttons[i].rect.w + (i ? BUTTON_GAP : 0);
    }

    m->icon_w = box->icon != UI_ICON_NONE ? UI_ICON_PX + ICON_GAP : 0;

    int wrap_w = box->width > 0 ? box->width - PAD * 2 - m->icon_w : TEXT_MAX_W;
    if (wrap_w < 1) wrap_w = 1;
    ui_text_measure(&ui_font_text, wrap_w, box->text ? box->text : "", &m->text_w, &m->text_h);

    // a width that was asked for is the width, the text gets all of it to be aligned in
    if (box->width > 0)
        m->text_w = wrap_w;

    m->body_h = m->text_h;
    if (m->icon_w && m->body_h < UI_ICON_PX)
        m->body_h = UI_ICON_PX;
}

static int window_width(const msgbox_t *m) {
    int w = m->icon_w + m->text_w;
    if (w < m->buttons_w) w = m->buttons_w;
    w += PAD * 2;
    return w < MIN_W ? MIN_W : w;
}

static int window_height(const msgbox_t *m) {
    return PAD + m->body_h + BUTTONS_TOP + BUTTON_H + PAD;
}

static void draw(cs_window_t *win, msgbox_t *m) {
    ui_canvas_t c = ui_canvas(win->pixels, win->width, win->height, win->pitch);

    ui_fill(&c, c.clip, UI_COLOUR_FACE);

    ui_icon(&c, m->box->icon, PAD, PAD + (m->body_h - UI_ICON_PX) / 2);

    ui_rect_t text = {PAD + m->icon_w, PAD + (m->body_h - m->text_h) / 2, m->text_w, m->text_h};
    ui_text_wrap(&c, &ui_font_text, text, m->box->text ? m->box->text : "", UI_COLOUR_TEXT, m->box->align);

    // the row of buttons is centred and stays on the bottom, whatever the window is made into
    int x = (win->width - m->buttons_w) / 2;
    int y = win->height - PAD - BUTTON_H;
    for (int i = 0; i < m->count; i++) {
        m->buttons[i].rect.x = x;
        m->buttons[i].rect.y = y;
        x += m->buttons[i].rect.w + BUTTON_GAP;

        if (i == m->box->default_button)
            ui_edge(&c, ui_rect_shrink(m->buttons[i].rect, -1), UI_COLOUR_DKSHADOW, UI_COLOUR_DKSHADOW);
        ui_button_draw(&c, &m->buttons[i]);
    }

    cs_window_commit(win, 0, 0, win->width, win->height);
}

int ui_msgbox(const ui_msgbox_t *box) {
    // nobody called ui_init, the fonts are needed before anything can be measured
    if (!ui_font_text.glyphs && ui_init() < 0)
        return UI_MSGBOX_FAILED;

    msgbox_t m;
    layout(&m, box);

    cs_window_t *win = cs_window_open(window_width(&m), window_height(&m), box->title ? box->title : "");
    if (!win)
        return UI_MSGBOX_FAILED;
    draw(win, &m);

    int closed = box->escape_button > 0 && box->escape_button <= m.count ? box->escape_button - 1
                                                                         : UI_MSGBOX_CLOSED;
    int result = UI_MSGBOX_FAILED;

    cs_event_t ev;
    while (result == UI_MSGBOX_FAILED) {
        cs_event_wait(&ev, -1);
        if (ev.window != win->id)
            continue;

        if (ev.type == CS_EV_CLOSE_REQUEST)
            result = closed;
        if (ev.type == CS_EV_RESIZE)
            draw(win, &m);

        if (ev.type == CS_EV_KEY && ev.key_action == KEY_IS_DOWN) {
            if (ev.keycode == KEY_ESCAPE)
                result = closed;
            if (ev.keycode == KEY_ENTER && box->default_button >= 0 && box->default_button < m.count)
                result = box->default_button;
        }

        if (ev.type == CS_EV_MOUSE) {
            int changed = 0;
            for (int i = 0; i < m.count; i++) {
                int did = ui_button_mouse(&m.buttons[i], ev.x, ev.y, ev.buttons & CS_BUTTON_LEFT);
                if (did & UI_CLICKED)
                    result = i;
                changed |= did & UI_CHANGED;
            }
            if (changed && result == UI_MSGBOX_FAILED)
                draw(win, &m);
        }
    }

    cs_window_close(win);
    return result;
}

void ui_message(const char *title, const char *text, int icon) {
    ui_msgbox_t box = { .title = title, .text = text, .icon = icon };
    ui_msgbox(&box);
}

int ui_ask(const char *title, const char *text) {
    ui_msgbox_t box = {
        .title          = title,
        .text           = text,
        .icon           = UI_ICON_QUESTION,
        .buttons        = { "Yes", "No" },
        .escape_button  = 2,
    };
    return ui_msgbox(&box) == 0;
}
