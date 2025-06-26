#ifndef MUSIC_DEFS_H
#define MUSIC_DEFS_H

#include <stdint.h>
#include "rand.h"

/* Defines musical scales, keys, and other seed-driven parameters */

typedef enum {
    SCALE_MAJOR_PENT,
    SCALE_MINOR_PENT
} scale_type_t;

typedef struct {
    float root_freq;
    scale_type_t scale_type;
    const int *scale_degrees;
    uint8_t scale_len;
} music_globals_t;

static const int PENT_MAJOR_DEGREES[] = {0,2,4,7,9};
static const int PENT_MINOR_DEGREES[] = {0,3,5,7,10};

static inline void music_globals_init(music_globals_t *g, rng_t *rng)
{
    float root_choices[] = {220.0f, 233.08f, 246.94f, 261.63f, 293.66f};
    g->root_freq = root_choices[rng_next_u32(rng) % 5];

    if(rng_next_u32(rng) % 2){
        g->scale_type = SCALE_MAJOR_PENT;
        g->scale_degrees = PENT_MAJOR_DEGREES;
        g->scale_len = sizeof(PENT_MAJOR_DEGREES)/sizeof(int);
    } else {
        g->scale_type = SCALE_MINOR_PENT;
        g->scale_degrees = PENT_MINOR_DEGREES;
        g->scale_len = sizeof(PENT_MINOR_DEGREES)/sizeof(int);
    }
}

#endif /* MUSIC_DEFS_H */ 