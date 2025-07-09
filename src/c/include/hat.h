#ifndef HAT_H
#define HAT_H

#include <stdint.h>
#include "rand.h"

typedef struct {
    uint32_t pos;
    uint32_t len;
    float32_t sr;
    float32_t env;
    float32_t env_coef;
    rng_t rng;
} hat_t;

void hat_init(hat_t *h, float32_t sr, uint64_t seed);
void hat_trigger(hat_t *h);
void hat_process(hat_t *h, float32_t *L, float32_t *R, uint32_t n);

#endif /* HAT_H */ 