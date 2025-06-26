#ifndef ENV_H
#define ENV_H

#include <stdint.h>

/* Generate exponential decay envelope into `out[n]`.
 * Formula: exp(-rate * t) where t in seconds.
 * Provide sample rate `sr` and decay constant `rate` (per second).
 */
void exp_env_block(float *out, uint32_t n, float rate, float sr);

/* simple exponential decay envelope */
static inline float32_t env_exp_decay(float32_t t, float32_t decay_rate)
{
    return expf(-decay_rate * t);
}

#endif /* ENV_H */ 