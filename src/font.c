#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <syscalls.h>
#include <bleed_uikit.h>
#include "file.h"
#include "ttf.h"

#define TEXT_PX     13
#define HEADING_PX  20
#define LIGHT_PX    14

ui_font_t ui_font_text;
ui_font_t ui_font_heading;
ui_font_t ui_font_light;

static int round_px(float v) {
    return (int)(v < 0 ? v - 0.5f : v + 0.5f);
}

int ui_font_load_file(ui_font_t *out, const char *path, int px) {
    *out = (ui_font_t){0};

    size_t   size;
    uint8_t *file = ui_file_read(path, &size);
    if (!file)
        return -1;

    ui_ttf_t ttf;
    if (ui_ttf_open(&ttf, file, size) < 0) {
        free(file);
        return -1;
    }

    float scale   = (float)px / (float)ttf.units;
    int   ascent  = round_px((float)ttf.ascent * scale);
    int   descent = round_px((float)ttf.descent * scale);

    ui_glyph_t *glyphs   = calloc(UI_FONT_GLYPHS, sizeof(ui_glyph_t));
    uint8_t    *coverage = NULL;
    size_t      used     = 0;
    size_t      cap      = 0;
    int         failed   = !glyphs;

    for (int i = 0; i < UI_FONT_GLYPHS && !failed; i++) {
        int glyph = ui_ttf_glyph(&ttf, (uint32_t)(UI_FONT_FIRST + i));
        int w, h, left, top;
        uint8_t *bits = ui_ttf_render(&ttf, glyph, scale, &w, &h, &left, &top);

        int advance = round_px((float)ui_ttf_advance(&ttf, glyph) * scale);
        glyphs[i].advance = (uint8_t)(advance > 255 ? 255 : advance);
        if (!bits)
            continue;

        // more than a glyph can say, the size asked for is silly
        if (w > 255 || h > 255 || left < -128 || left > 127 || ascent - top < -128 || ascent - top > 127) {
            free(bits);
            failed = 1;
            break;
        }

        size_t bytes = (size_t)w * h;
        if (used + bytes > cap) {
            cap = (used + bytes) * 2;
            uint8_t *bigger = realloc(coverage, cap);
            if (!bigger) {
                free(bits);
                failed = 1;
                break;
            }
            coverage = bigger;
        }
        memcpy(coverage + used, bits, bytes);
        free(bits);

        glyphs[i].w      = (uint8_t)w;
        glyphs[i].h      = (uint8_t)h;
        glyphs[i].left   = (int8_t)left;
        glyphs[i].top    = (int8_t)(ascent - top);
        glyphs[i].offset = (uint32_t)used;
        used += bytes;
    }
    free(file);

    if (failed || !coverage) {
        free(glyphs);
        free(coverage);
        return -1;
    }

    out->line_height = ascent + descent;
    out->ascent      = ascent;
    out->glyphs      = glyphs;
    out->coverage    = coverage;
    return 0;
}

int ui_font_load(ui_font_t *out, const char *role, int px) {
    *out = (ui_font_t){0};

    int dir = _open(UI_FONT_DIR, O_RDONLY);
    if (dir < 0)
        return -1;

    dirent_t entry;
    char     path[sizeof(UI_FONT_DIR) + sizeof(entry.name) + 1];
    size_t   role_len = strlen(role);
    int      found    = 0;

    for (size_t i = 0; !found && _readdir(dir, i, &entry) == 0; i++) {
        entry.name[sizeof(entry.name) - 1] = '\0';
        size_t len = strlen(entry.name);
        if (len < role_len || strcmp(entry.name + len - role_len, role) != 0)
            continue;

        strcpy(path, UI_FONT_DIR "/");
        strcat(path, entry.name);
        found = 1;
    }
    _close(dir);

    return found ? ui_font_load_file(out, path, px) : -1;
}

// the font for role, or the regular one at that size if theres no file for it
static int load_or_regular(ui_font_t *out, const char *role, int px) {
    if (ui_font_load(out, role, px) == 0)
        return 0;
    return ui_font_load(out, UI_FONT_REGULAR, px);
}

int ui_init(void) {
    if (ui_font_load(&ui_font_text, UI_FONT_REGULAR, TEXT_PX) < 0)
        return -1;
    if (load_or_regular(&ui_font_heading, UI_FONT_BOLD, HEADING_PX) < 0)
        return -1;
    return load_or_regular(&ui_font_light, UI_FONT_LIGHT, LIGHT_PX);
}

static const ui_glyph_t *glyph_for(const ui_font_t *font, char c) {
    // a font that never loaded draws nothing
    static const ui_glyph_t none;
    if (!font->glyphs)
        return &none;

    if (c < UI_FONT_FIRST || c >= UI_FONT_FIRST + UI_FONT_GLYPHS)
        c = '?';
    return &font->glyphs[c - UI_FONT_FIRST];
}

int ui_font_height(const ui_font_t *font) {
    return font->line_height;
}

int ui_text_width(const ui_font_t *font, const char *text) {
    int width = 0;
    for (; *text; text++)
        width += glyph_for(font, *text)->advance;
    return width;
}

// simple blend
static inline uint32_t blend(uint32_t under, uint32_t over, uint32_t alpha) {
    if (alpha == 255)
        return over;

    uint32_t rb = (over & 0xFF00FF) * alpha + (under & 0xFF00FF) * (255 - alpha);
    uint32_t g  = ((over >> 8) & 0xFF) * alpha + ((under >> 8) & 0xFF) * (255 - alpha);

    rb = ((rb + 0x010001 + ((rb >> 8) & 0xFF00FF)) >> 8) & 0xFF00FF;
    g  = (g + 1 + (g >> 8)) >> 8;
    return rb | (g << 8);
}

void ui_text(ui_canvas_t *c, const ui_font_t *font, int x, int y, const char *text, uint32_t colour) {
    ui_rect_t clip = c->clip;

    for (; *text; text++) {
        const ui_glyph_t *g    = glyph_for(font, *text);
        const uint8_t    *bits = font->coverage + g->offset;
        int               gx   = x + g->left;
        int               gy   = y + g->top;

        // only the part of the glyph clip shows, worked out once instead of asked per pixel
        int col0 = clip.x > gx ? clip.x - gx : 0;
        int row0 = clip.y > gy ? clip.y - gy : 0;
        int col1 = clip.x + clip.w - gx < g->w ? clip.x + clip.w - gx : g->w;
        int row1 = clip.y + clip.h - gy < g->h ? clip.y + clip.h - gy : g->h;

        for (int row = row0; row < row1; row++) {
            const uint8_t *cover = bits + row * g->w;
            uint32_t      *dst   = c->pixels + (size_t)(gy + row) * c->pitch + gx;

            for (int col = col0; col < col1; col++) {
                if (cover[col])
                    dst[col] = blend(dst[col], colour, cover[col]);
            }
        }
        x += g->advance;
    }
}

void ui_text_in(ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align) {
    int x = r.x;
    if (align == UI_ALIGN_CENTRE) x = r.x + (r.w - ui_text_width(font, text)) / 2;
    if (align == UI_ALIGN_RIGHT)  x = r.x + r.w - ui_text_width(font, text);

    ui_text(c, font, x, r.y + (r.h - ui_font_height(font)) / 2, text, colour);
}

#define WRAP_LINE_MAX 256

// how much of text goes on one line max_w across. len is how many characters to draw and
// next where the line after starts, past the space or newline that ended this one
static int wrap_line(const ui_font_t *font, const char *text, int max_w, size_t *len, size_t *next) {
    int    width      = 0;
    int    space_w    = 0;
    size_t space      = 0;
    size_t i          = 0;

    for (; text[i] && text[i] != '\n' && i < WRAP_LINE_MAX - 1; i++) {
        int advance = glyph_for(font, text[i])->advance;

        if (max_w > 0 && width + advance > max_w && i > 0) {
            // no space to break at and its cut mid word, theres nowhere better
            if (space) {
                *len  = space;
                *next = space + 1;
                return space_w;
            }
            *len  = i;
            *next = i;
            return width;
        }
        if (text[i] == ' ') {
            space   = i;
            space_w = width;
        }
        width += advance;
    }

    *len  = i;
    *next = text[i] == '\n' ? i + 1 : i;
    return width;
}

static int wrap(ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour,
                int align, int *widest) {
    char line[WRAP_LINE_MAX];
    int  y = r.y;

    while (*text) {
        size_t len, next;
        int    width = wrap_line(font, text, r.w, &len, &next);
        if (widest && width > *widest)
            *widest = width;

        if (c) {
            memcpy(line, text, len);
            line[len] = '\0';
            ui_text_in(c, font, (ui_rect_t){r.x, y, r.w, ui_font_height(font)}, line, colour, align);
        }
        y += ui_font_height(font);

        // a line that took nothing would go round forever
        if (!next)
            break;
        text += next;
    }
    return y - r.y;
}

int ui_text_wrap(ui_canvas_t *c, const ui_font_t *font, ui_rect_t r, const char *text, uint32_t colour, int align) {
    return wrap(c, font, r, text, colour, align, NULL);
}

void ui_text_measure(const ui_font_t *font, int max_w, const char *text, int *w, int *h) {
    int widest = 0;
    int height = wrap(NULL, font, (ui_rect_t){0, 0, max_w, 0}, text, 0, UI_ALIGN_LEFT, &widest);

    if (w) *w = widest;
    if (h) *h = height;
}
