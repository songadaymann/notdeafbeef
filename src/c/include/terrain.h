#ifndef TERRAIN_H
#define TERRAIN_H

#include <stdint.h>
#include <stdbool.h>
#include "raster.h"
#include "rand.h"

#define TILE_SIZE 32
#define TERRAIN_LEN 64

/* Tile types */
enum { TILE_FLAT=0, TILE_WALL=1, TILE_SLOPE_UP=2, TILE_SLOPE_DOWN=3, TILE_GAP=4 };

typedef struct {
    uint8_t type;
    uint8_t height; /* rows of tiles */
} terrain_tile_t;

void terrain_init(uint64_t seed);
void terrain_draw(uint32_t *fb, int w, int h, int frame);

#endif /* TERRAIN_H */ 