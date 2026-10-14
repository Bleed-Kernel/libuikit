#pragma once
#include <stdint.h>
#include <stddef.h>

// a whole png file in, w * h pixels of 0xAARRGGBB out for the caller to free. NULL if it isnt
// a png or its an interlaced one, thats the only kind this cant read
uint32_t *ui_png_decode(const uint8_t *data, size_t size, int *w, int *h);
