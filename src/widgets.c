#include <bleed_uikit.h>

void ui_panel(ui_canvas_t *c, ui_rect_t r, int style) {
    if (style == UI_SUNKEN) {
        ui_fill(c, r, UI_COLOUR_BACK_L);
        ui_edge(c, r, UI_COLOUR_SHADOW, UI_COLOUR_HILIGHT);
        ui_edge(c, ui_rect_shrink(r, 1), UI_COLOUR_DKSHADOW, UI_COLOUR_LIGHT);
        return;
    }

    ui_fill(c, r, UI_COLOUR_FACE);
    ui_edge(c, r, UI_COLOUR_LIGHT, UI_COLOUR_DKSHADOW);
    ui_edge(c, ui_rect_shrink(r, 1), UI_COLOUR_HILIGHT, UI_COLOUR_SHADOW);
}

void ui_separator(ui_canvas_t *c, int x, int y, int w) {
    ui_fill(c, (ui_rect_t){x, y, w, 1}, UI_COLOUR_SHADOW);
    ui_fill(c, (ui_rect_t){x, y + 1, w, 1}, UI_COLOUR_HILIGHT);
}

void ui_progress(ui_canvas_t *c, ui_rect_t r, int permille) {
    if (permille < 0)    permille = 0;
    if (permille > 1000) permille = 1000;

    ui_panel(c, r, UI_SUNKEN);

    // the gradient runs the whole length and is cut off, so the colour at any point doesnt
    // move as the bar fills
    ui_rect_t inside = ui_rect_shrink(r, 2);
    ui_rect_t old    = c->clip;
    ui_clip(c, ui_rect_clip((ui_rect_t){inside.x, inside.y, inside.w * permille / 1000, inside.h}, old));
    ui_gradient(c, inside, UI_COLOUR_CAPTION_L, UI_COLOUR_CAPTION_R, 0);
    c->clip = old;
}

void ui_button_draw(ui_canvas_t *c, const ui_button_t *b) {
    ui_rect_t r    = b->rect;
    int       down = b->held && b->hover;

    ui_fill(c, r, down ? UI_COLOUR_LIGHT : b->hover ? UI_COLOUR_BUTTON_LIT : UI_COLOUR_BUTTON);
    if (down) {
        ui_edge(c, r, UI_COLOUR_BUTTON_LO, UI_COLOUR_BUTTON_HI);
        ui_edge(c, ui_rect_shrink(r, 1), UI_COLOUR_BUTTON_IN_LO, UI_COLOUR_BUTTON_IN_HI);
    } else {
        ui_edge(c, r, UI_COLOUR_BUTTON_HI, UI_COLOUR_BUTTON_LO);
        ui_edge(c, ui_rect_shrink(r, 1), UI_COLOUR_BUTTON_IN_HI, UI_COLOUR_BUTTON_IN_LO);
    }

    // the label goes down and right a pixel with the button
    ui_rect_t old   = c->clip;
    ui_rect_t label = ui_rect_shrink(r, 2);
    ui_clip(c, ui_rect_clip(label, old));
    label.x += down;
    label.y += down;
    ui_text_in(c, &ui_font_text, label, b->label, UI_COLOUR_TEXT, UI_ALIGN_CENTRE);
    c->clip = old;
}

int ui_button_mouse(ui_button_t *b, int x, int y, int left) {
    uint8_t hover  = (uint8_t)ui_rect_contains(b->rect, x, y);
    uint8_t held   = b->held;
    int     result = 0;

    // only a press that starts on the button counts, dragging onto it with the mouse down doesnt
    if (left && !b->was_left && hover)
        held = 1;
    if (!left) {
        if (held && hover)
            result |= UI_CLICKED;
        held = 0;
    }

    if (hover != b->hover || held != b->held)
        result |= UI_CHANGED;

    b->hover    = hover;
    b->held     = held;
    b->was_left = (uint8_t)(left != 0);
    return result;
}
