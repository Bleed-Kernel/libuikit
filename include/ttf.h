#pragma once
#include <stdint.h>
#include <stddef.h>

// a truetype font file
typedef struct {
    const uint8_t *data;
    size_t         size;

    uint32_t glyf, loca, hmtx;
    uint32_t cmap;          // the one subtable thats used
    int      cmap_format;   // 4 or 12

    int units;              // font units to the em
    int loca_long;
    int glyphs;
    int hmetrics;

    // font units, descent is how far below the baseline as a positive number
    int ascent;
    int descent;
} ui_ttf_t;

// 0 if data is a font this can draw. data has to stay around as long as t does
int ui_ttf_open(ui_ttf_t *t, const uint8_t *data, size_t size);

// which glyph a character is, 0 is the fonts own missing glyph
int ui_ttf_glyph(const ui_ttf_t *t, uint32_t codepoint);

// how far the pen moves after a glyph, in font units
int ui_ttf_advance(const ui_ttf_t *t, int glyph);

uint8_t *ui_ttf_render(const ui_ttf_t *t, int glyph, float scale, int *w, int *h, int *left, int *top);
