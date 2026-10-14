#include <stdlib.h>
#include <string.h>
#include <bleed_uikit.h>
#include "file.h"
#include "png.h"

int ui_image_load(ui_image_t *img, const char *path) {
    size_t   size;
    uint8_t *file = ui_file_read(path, &size);
    if (!file)
        return -1;

    img->pixels = ui_png_decode(file, size, &img->w, &img->h);
    free(file);
    return img->pixels ? 0 : -1;
}

int ui_image_load_raw(ui_image_t *img, const char *path, int w, int h) {
    size_t   size;
    uint8_t *file = ui_file_read(path, &size);
    if (!file)
        return -1;

    if (w <= 0 || h <= 0 || size != (size_t)w * h * 4) {
        free(file);
        return -1;
    }

    // b g r a in a row is a little endian 0xAARRGGBB, the file is the pixels as they are
    img->pixels = (uint32_t *)file;
    img->w      = w;
    img->h      = h;
    return 0;
}

void ui_image_free(ui_image_t *img) {
    free(img->pixels);
    memset(img, 0, sizeof(*img));
}

int ui_image_scale(ui_image_t *img, int w, int h) {
    if (w <= 0 || h <= 0 || !img->pixels)
        return -1;

    uint32_t *out = malloc((size_t)w * h * sizeof(uint32_t));
    if (!out)
        return -1;

    for (int y = 0; y < h; y++) {
        // the rows and columns of the old one this pixel lies over, never less than one of each
        int y0 = y * img->h / h;
        int y1 = (y + 1) * img->h / h;
        if (y1 <= y0) y1 = y0 + 1;

        for (int x = 0; x < w; x++) {
            int x0 = x * img->w / w;
            int x1 = (x + 1) * img->w / w;
            if (x1 <= x0) x1 = x0 + 1;

            uint32_t sum[4] = {0};
            for (int sy = y0; sy < y1; sy++) {
                for (int sx = x0; sx < x1; sx++) {
                    uint32_t p = img->pixels[(size_t)sy * img->w + sx];
                    for (int i = 0; i < 4; i++)
                        sum[i] += (p >> (i * 8)) & 0xFF;
                }
            }

            uint32_t count = (uint32_t)((y1 - y0) * (x1 - x0));
            uint32_t pixel = 0;
            for (int i = 0; i < 4; i++)
                pixel |= (sum[i] / count) << (i * 8);
            out[(size_t)y * w + x] = pixel;
        }
    }

    free(img->pixels);
    img->pixels = out;
    img->w      = w;
    img->h      = h;
    return 0;
}

void ui_image_key_black(ui_image_t *img) {
    for (size_t i = 0; i < (size_t)img->w * img->h; i++) {
        uint32_t p = img->pixels[i];
        uint32_t r = (p >> 16) & 0xFF, g = (p >> 8) & 0xFF, b = p & 0xFF;
        uint32_t most = r > g ? r : g;
        if (b > most) most = b;

        // the colour is brought up to full as its made see through, blended back it comes out the same
        if (most) {
            r = r * 255 / most;
            g = g * 255 / most;
            b = b * 255 / most;
        }
        img->pixels[i] = (most << 24) | (r << 16) | (g << 8) | b;
    }
}

void ui_image_draw(ui_canvas_t *c, const ui_image_t *img, int x, int y) {
    if (!img->pixels)
        return;
    ui_rect_t r = ui_rect_clip((ui_rect_t){x, y, img->w, img->h}, c->clip);

    for (int row = r.y; row < r.y + r.h; row++) {
        const uint32_t *src = img->pixels + (size_t)(row - y) * img->w + (r.x - x);
        uint32_t       *dst = c->pixels + (size_t)row * c->pitch + r.x;

        for (int col = 0; col < r.w; col++) {
            uint32_t alpha = src[col] >> 24;
            if (!alpha)
                continue;
            if (alpha == 255) {
                dst[col] = src[col] & 0xFFFFFF;
                continue;
            }

            uint32_t out = 0;
            for (int shift = 0; shift < 24; shift += 8) {
                uint32_t over  = (src[col] >> shift) & 0xFF;
                uint32_t under = (dst[col] >> shift) & 0xFF;
                out |= ((over * alpha + under * (255 - alpha) + 127) / 255) << shift;
            }
            dst[col] = out;
        }
    }
}
