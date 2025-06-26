#ifndef CRT_FX_H
#define CRT_FX_H

#include <stdint.h>
#include "rand.h"

typedef struct {
    /* persistence (ghost trails) */
    uint32_t *prev_frame;
    float persistence;      /* 0.3 (heavy) to 0.9 (minimal) */
    
    /* scanlines */
    int scanline_alpha;     /* 0-200 */
    
    /* chromatic aberration */
    int chroma_shift;       /* 0-5 pixels */
    
    /* noise */
    int noise_pixels;       /* 0-300 per frame */
    
    /* jitter */
    float jitter_amount;    /* 0-3 pixels */
    
    /* frame drops */
    float frame_drop_chance; /* 0-0.1 */
    
    /* color bleed */
    float color_bleed;      /* 0-0.3 */
    
    /* RNG for effects */
    rng_t rng;
} crt_fx_t;

void crt_fx_init(crt_fx_t *fx, uint64_t seed, int w, int h);
void crt_fx_apply(crt_fx_t *fx, uint32_t *fb, int w, int h, int frame);
void crt_fx_cleanup(crt_fx_t *fx);

#endif /* CRT_FX_H */ 