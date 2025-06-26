#ifndef SHAPES_H
#define SHAPES_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_SHAPES 8

typedef enum {
    SHAPE_TRIANGLE,
    SHAPE_DIAMOND,
    SHAPE_HEXAGON,
    SHAPE_STAR,
    SHAPE_SQUARE
} shape_type_t;

typedef struct {
    shape_type_t type;
    float scale;      /* 0.1 to 1.0 */
    float rotation;   /* radians */
    float rot_speed;  /* radians per frame */
    int alpha;        /* 0-255 */
    uint32_t color;
} bass_shape_t;

void shapes_init(void);
void shapes_spawn(shape_type_t type, uint32_t color);
void shapes_update_and_draw(uint32_t *fb, int w, int h);

#endif /* SHAPES_H */ 