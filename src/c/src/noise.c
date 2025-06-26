#include "noise.h"

void noise_block(rng_t *rng, float *out, uint32_t n)
{
    for (uint32_t i = 0; i < n; ++i) {
        // rng_next_float returns [0,1). Map to [-1,1)
        float v = rng_next_float(rng) * 2.0f - 1.0f;
        out[i] = v;
    }
} 