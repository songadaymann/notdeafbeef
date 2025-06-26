#include "terrain.h"
#include <string.h>
#include <math.h>

/* --- internal static assets --- */
static uint32_t g_tile_flat[TILE_SIZE*TILE_SIZE];
static uint32_t g_tile_slope_up[TILE_SIZE*TILE_SIZE];
static uint32_t g_tile_slope_down[TILE_SIZE*TILE_SIZE];

static terrain_tile_t g_pattern[TERRAIN_LEN];

/* pack rgb8 + alpha 255 into uint32 */
static inline uint32_t rgba(uint8_t r,uint8_t g,uint8_t b){ return ((uint32_t)r<<24)|((uint32_t)g<<16)|((uint32_t)b<<8)|0xFF; }

static void build_rock_tile(uint32_t *dst, uint8_t base)
{
    uint8_t dark = base;
    uint8_t mid  = (uint8_t)fminf(base*1.3f, 255.0f);
    uint8_t hi   = (uint8_t)fminf(base*1.8f, 255.0f);
    for(int y=0;y<TILE_SIZE;y++){
        for(int x=0;x<TILE_SIZE;x++){
            uint8_t h = (uint8_t)(((x*13 + y*7) ^ (x>>3)) & 0xFF);
            uint32_t col;
            if(h < 40)      col = rgba(hi,hi,hi);
            else if(h < 120) col = rgba(mid,mid,mid);
            else              col = rgba(dark,dark,dark);
            dst[y*TILE_SIZE + x] = col;
        }
    }
}

static void build_slope_tile(uint32_t *dst,const uint32_t *rock,uint8_t direction) /* 0=up,1=down */
{
    for(int y=0;y<TILE_SIZE;y++){
        for(int x=0;x<TILE_SIZE;x++){
            int threshold = (direction==0)? x : (TILE_SIZE-1 - x);
            if(y > threshold){
                dst[y*TILE_SIZE + x] = rock[y*TILE_SIZE + x];
            } else {
                dst[y*TILE_SIZE + x] = 0x00000000; /* transparent */
            }
        }
    }
}

void terrain_init(uint64_t seed)
{
    /* choose base grey based on seed for determinism */
    rng_t rng = rng_seed(seed ^ 0xA17E44ULL);
    uint8_t base = (uint8_t)(64 + (rng_next_u32(&rng) % 96));
    build_rock_tile(g_tile_flat, base);
    build_slope_tile(g_tile_slope_up, g_tile_flat, 0);
    build_slope_tile(g_tile_slope_down, g_tile_flat, 1);

    /* --- pattern generation --- */
    size_t i=0;
    while(i < TERRAIN_LEN){
        uint32_t feature = rng_next_u32(&rng) % 5; /* 0..4 */
        if(feature == 0){ /* flat */
            int length = 2 + (rng_next_u32(&rng)%5); /* 2-6 */
            for(int k=0;k<length && i<TERRAIN_LEN;k++){
                g_pattern[i++] = (terrain_tile_t){TILE_FLAT, 2};
            }
        } else if(feature == 1){ /* wall */
            int wall_h = (rng_next_u32(&rng)%2)?4:6;
            int wall_w = 2 + (rng_next_u32(&rng)%3); /*2-4*/
            for(int k=0;k<wall_w && i<TERRAIN_LEN;k++){
                g_pattern[i++] = (terrain_tile_t){TILE_WALL, (uint8_t)wall_h};
            }
        } else if(feature == 2){ /* slope up then platform */
            if(i<TERRAIN_LEN){ g_pattern[i++] = (terrain_tile_t){TILE_SLOPE_UP,2}; }
            int length = 2 + (rng_next_u32(&rng)%3); /*2-4*/
            for(int k=0;k<length && i<TERRAIN_LEN;k++){
                g_pattern[i++] = (terrain_tile_t){TILE_FLAT,3};
            }
        } else if(feature == 3){ /* slope down */
            if(i<TERRAIN_LEN){ g_pattern[i++] = (terrain_tile_t){TILE_SLOPE_DOWN,3}; }
        } else { /* gap */
            int length = 1 + (rng_next_u32(&rng)%2); /*1-2*/
            for(int k=0;k<length && i<TERRAIN_LEN;k++){
                g_pattern[i++] = (terrain_tile_t){TILE_GAP,0};
            }
        }
    }
}

void terrain_draw(uint32_t *fb,int w,int h,int frame)
{
    const int SCROLL_SPEED = 2; /* pixels/frame */
    int offset_px = (frame * SCROLL_SPEED) % TILE_SIZE;
    int scroll_tiles = (frame * SCROLL_SPEED) / TILE_SIZE;
    int tiles_per_screen = w / TILE_SIZE + 2;

    for(int i=0;i<tiles_per_screen;i++){
        int terrain_idx = (scroll_tiles + i) % TERRAIN_LEN;
        terrain_tile_t tile = g_pattern[terrain_idx];
        int x0 = i*TILE_SIZE - offset_px;
        if(tile.type == TILE_GAP) continue;

        for(int row=0; row<tile.height; ++row){
            int y0 = h - (row+1)*TILE_SIZE;
            const uint32_t *src_px;
            if(tile.type == TILE_SLOPE_UP && row==tile.height-1){
                src_px = g_tile_slope_up;
            } else if(tile.type == TILE_SLOPE_DOWN && row==tile.height-1){
                src_px = g_tile_slope_down;
            } else {
                src_px = g_tile_flat;
            }
            /* choose blit with alpha for slope tiles (transparent parts) */
            bool use_alpha = (src_px == g_tile_slope_up || src_px == g_tile_slope_down);
            if(use_alpha){
                raster_blit_rgba_alpha(src_px, TILE_SIZE, TILE_SIZE, fb, w, h, x0, y0);
            } else {
                raster_blit_rgba(src_px, TILE_SIZE, TILE_SIZE, fb, w, h, x0, y0);
            }
        }
    }
} 