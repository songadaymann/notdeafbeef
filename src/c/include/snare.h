#ifndef SNARE_H
#define SNARE_H

#include <stdint.h>
#include "rand.h"

typedef struct {
    uint32_t pos;
    uint32_t len;
    float32_t sr;
    float32_t env;        /* current envelope value */
    float32_t env_coef;   /* per-sample decay coefficient */
    rng_t rng;
} snare_t;

void snare_init(snare_t *s, float32_t sr, uint64_t seed);
void snare_trigger(snare_t *s);
void snare_process(snare_t *s, float32_t *L, float32_t *R, uint32_t n);

#endif /* SNARE_H */ 