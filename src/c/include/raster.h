#ifndef RASTER_H
#define RASTER_H
#include <stdint.h>
#include <stdbool.h>

void raster_clear(uint32_t *fb, int w, int h, uint32_t color_rgba);
void raster_circle(uint32_t *fb, int w, int h, int cx, int cy, int r, uint32_t color_rgba, int thickness);
void raster_fill_circle(uint32_t *fb, int w, int h, int cx, int cy, int r, uint32_t color_rgba);
void raster_line(uint32_t *fb, int w, int h, int x0, int y0, int x1, int y1, uint32_t color_rgba);
void raster_poly(uint32_t *fb, int w, int h, const int *vx, const int *vy, int n, uint32_t color_rgba, bool fill, int thickness);
void raster_blit_rgba(const uint32_t *src, int src_w, int src_h, uint32_t *dst_fb, int dst_w, int dst_h, int dx, int dy);
void raster_blit_rgba_alpha(const uint32_t *src, int src_w, int src_h, uint32_t *dst_fb, int dst_w, int dst_h, int dx, int dy);

#endif 