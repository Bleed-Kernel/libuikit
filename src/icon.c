#include <bleed_uikit.h>

#define SUB     4                       // samples each way inside a pixel
#define UNIT    (UI_ICON_PX * SUB)      // the sign measured in samples

// x y in samples from the top left of the sign
static int in_circle(int x, int y) {
    int dx = x * 2 + 1 - UNIT;
    int dy = y * 2 + 1 - UNIT;
    int r  = UNIT - SUB * 2;
    return dx * dx + dy * dy <= r * r;
}

// point up, a little in from the box all round
static int in_triangle(int x, int y) {
    int top    = SUB * 2;
    int bottom = UNIT - SUB * 2;
    if (y < top || y >= bottom)
        return 0;

    int half = (y - top) * (UNIT / 2 - SUB) / (bottom - top);
    int dx   = x - UNIT / 2;
    return dx >= -half - 1 && dx <= half;
}

static uint32_t mix(uint32_t under, uint32_t over, uint32_t part, uint32_t whole) {
    uint32_t out = 0;
    for (int shift = 0; shift < 24; shift += 8) {
        uint32_t a = (over >> shift) & 0xFF;
        uint32_t b = (under >> shift) & 0xFF;
        out |= ((a * part + b * (whole - part)) / whole) << shift;
    }
    return out;
}

void ui_icon(ui_canvas_t *c, int icon, int x, int y) {
    const char *mark;
    uint32_t    colour;
    uint32_t    ink      = 0xFFFFFF;
    int         triangle = 0;

    switch (icon) {
    case UI_ICON_INFO:     mark = "i"; colour = UI_COLOUR_INFO; break;
    case UI_ICON_QUESTION: mark = "?"; colour = UI_COLOUR_INFO; break;
    case UI_ICON_ERROR:    mark = "X"; colour = UI_COLOUR_HOT;  break;
    case UI_ICON_WARNING:
        mark     = "!";
        colour   = UI_COLOUR_WARN;
        ink      = UI_COLOUR_SIGN_DARK;
        triangle = 1;
        break;
    default:
        return;
    }

    // how many of a pixels samples land in the shape is how solid it is, thats the smooth edge
    for (int row = 0; row < UI_ICON_PX; row++) {
        for (int col = 0; col < UI_ICON_PX; col++) {
            if (!ui_rect_contains(c->clip, x + col, y + row))
                continue;

            uint32_t hits = 0;
            for (int sy = 0; sy < SUB; sy++) {
                for (int sx = 0; sx < SUB; sx++) {
                    int px = col * SUB + sx;
                    int py = row * SUB + sy;
                    hits += triangle ? in_triangle(px, py) : in_circle(px, py);
                }
            }
            if (!hits)
                continue;

            uint32_t *dst = c->pixels + (size_t)(y + row) * c->pitch + x + col;
            *dst = mix(*dst, colour, hits, SUB * SUB);
        }
    }

    ui_rect_t box = {x, y + (triangle ? 4 : 0), UI_ICON_PX, UI_ICON_PX - (triangle ? 4 : 0)};
    ui_text_in(c, &ui_font_heading, box, mark, ink, UI_ALIGN_CENTRE);
}
