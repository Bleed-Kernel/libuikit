#include <bleed_uikit.h>

ui_canvas_t ui_canvas(uint32_t *pixels, int width, int height, int pitch) {
    return (ui_canvas_t){
        .pixels = pixels,
        .width  = width,
        .height = height,
        .pitch  = pitch,
        .clip   = {0, 0, width, height},
    };
}

void ui_clip(ui_canvas_t *c, ui_rect_t r) {
    c->clip = ui_rect_clip(r, (ui_rect_t){0, 0, c->width, c->height});
}

void ui_unclip(ui_canvas_t *c) {
    c->clip = (ui_rect_t){0, 0, c->width, c->height};
}

ui_rect_t ui_rect_clip(ui_rect_t r, ui_rect_t bounds) {
    int x0 = r.x > bounds.x ? r.x : bounds.x;
    int y0 = r.y > bounds.y ? r.y : bounds.y;
    int x1 = r.x + r.w < bounds.x + bounds.w ? r.x + r.w : bounds.x + bounds.w;
    int y1 = r.y + r.h < bounds.y + bounds.h ? r.y + r.h : bounds.y + bounds.h;

    if (x1 <= x0 || y1 <= y0)
        return (ui_rect_t){0, 0, 0, 0};
    return (ui_rect_t){x0, y0, x1 - x0, y1 - y0};
}

ui_rect_t ui_rect_shrink(ui_rect_t r, int by) {
    return (ui_rect_t){r.x + by, r.y + by, r.w - by * 2, r.h - by * 2};
}

int ui_rect_contains(ui_rect_t r, int x, int y) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

void ui_fill(ui_canvas_t *c, ui_rect_t r, uint32_t colour) {
    r = ui_rect_clip(r, c->clip);

    for (int y = r.y; y < r.y + r.h; y++) {
        uint32_t *row = c->pixels + (size_t)y * c->pitch + r.x;
        for (int x = 0; x < r.w; x++)
            row[x] = colour;
    }
}

// the bottom right colour gets the two corners like the real thing
void ui_edge(ui_canvas_t *c, ui_rect_t r, uint32_t top_left, uint32_t bottom_right) {
    ui_fill(c, (ui_rect_t){r.x, r.y, r.w - 1, 1}, top_left);
    ui_fill(c, (ui_rect_t){r.x, r.y, 1, r.h - 1}, top_left);
    ui_fill(c, (ui_rect_t){r.x, r.y + r.h - 1, r.w, 1}, bottom_right);
    ui_fill(c, (ui_rect_t){r.x + r.w - 1, r.y, 1, r.h}, bottom_right);
}

// ordered dither, without it a slow gradient shows up as stripes
static const uint8_t bayer[4][4] = {
    {   0, 128,  32, 160 },
    { 192,  64, 224,  96 },
    {  48, 176,  16, 144 },
    { 240, 112, 208,  80 },
};

void ui_gradient(ui_canvas_t *c, ui_rect_t r, uint32_t from, uint32_t to, int vertical) {
    ui_rect_t clip = ui_rect_clip(r, c->clip);
    int       span = (vertical ? r.h : r.w) - 1;
    if (span < 1) span = 1;

    // where each channel starts and how far a step moves it, times 65536
    int base[3], step[3];
    for (int i = 0; i < 3; i++) {
        int a = (from >> (i * 8)) & 0xFF;
        int b = (to >> (i * 8)) & 0xFF;
        base[i] = a << 16;
        step[i] = ((b - a) << 16) / span;
    }

    for (int y = clip.y; y < clip.y + clip.h; y++) {
        uint32_t *row = c->pixels + (size_t)y * c->pitch;

        for (int x = clip.x; x < clip.x + clip.w; x++) {
            int      t      = vertical ? y - r.y : x - r.x;
            uint32_t colour = 0;

            for (int i = 0; i < 3; i++) {
                int v = (((base[i] + step[i] * t) >> 8) + bayer[y & 3][x & 3]) >> 8;
                if (v > 255) v = 255;
                colour |= (uint32_t)v << (i * 8);
            }
            row[x] = colour;
        }
    }
}
