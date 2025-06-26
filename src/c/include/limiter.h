#ifndef LIMITER_H
#define LIMITER_H

#include <stdint.h>
#include <math.h>

/* A simple soft-knee stereo limiter. */

typedef struct {
    float32_t attack_coeff;
    float32_t release_coeff;
    float32_t envelope;
    float32_t threshold;
    float32_t knee_width;
} limiter_t;

static inline void limiter_init(limiter_t *l, float32_t sr, float32_t attack_ms, float32_t release_ms, float32_t threshold_db)
{
    l->attack_coeff = expf(-1.0f / (attack_ms * sr / 1000.0f));
    l->release_coeff = expf(-1.0f / (release_ms * sr / 1000.0f));
    l->envelope = 0.0f;
    l->threshold = powf(10.0f, threshold_db / 20.0f);
    l->knee_width = 5.0f; // 5dB soft knee
}

void limiter_process(limiter_t *l, float32_t *L, float32_t *R, uint32_t n);

#endif /* LIMITER_H */ 