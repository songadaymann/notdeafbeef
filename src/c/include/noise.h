#ifndef NOISE_H
#define NOISE_H

#include <stdint.h>
#include "rand.h"

/* Fill `out[n]` with white noise in range [-1,1). Uses provided RNG. */
void noise_block(rng_t *rng, float *out, uint32_t n);

#endif /* NOISE_H */ 