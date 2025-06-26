#ifndef RAND_H
#define RAND_H

#include <stdint.h>

/* Simple 64-bit SplitMix64 PRNG seeded with any 64-bit value. */
typedef struct {
    uint64_t state;
} rng_t;

static inline rng_t rng_seed(uint64_t seed)
{
    rng_t r; r.state = seed + 0x9E3779B97F4A7C15ULL; return r;
}

/* Produce next 64-bit pseudo-random value. */
static inline uint64_t rng_next_u64(rng_t *r)
{
    uint64_t z = (r->state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static inline uint32_t rng_next_u32(rng_t *r)
{
    return (uint32_t)rng_next_u64(r);
}

/* Return float in [0,1). */
static inline float rng_next_float(rng_t *r)
{
    return (rng_next_u32(r) >> 8) * (1.0f / 16777216.0f); // 24-bit mantissa
}

static inline float rng_float_mono(rng_t *r)
{
    return rng_next_float(r) * 2.0f - 1.0f; /* -1..1 */
}

#endif /* RAND_H */ 