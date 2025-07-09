#include "snare.h"
#include <math.h>

#define TAU 6.28318530717958647692f
#define SNARE_DECAY_RATE 35.0f
#define SNARE_DUR_SEC 0.1f
#define SNARE_AMP 0.4f

void snare_init(snare_t *s, float32_t sr, uint64_t seed)
{
    s->len = 0;
    s->pos = 0;
    s->sr = sr;
    s->env = 0.0f;
    s->env_coef = 0.0f;
    s->rng = rng_seed(seed);
}

void snare_trigger(snare_t *s)
{
    s->pos = 0;
    s->len = (uint32_t)(SNARE_DUR_SEC * s->sr);
    s->env = 1.0f;
    s->env_coef = expf(-SNARE_DECAY_RATE / s->sr);
}

#ifndef SNARE_ASM
void snare_process(snare_t *s, float32_t *L, float32_t *R, uint32_t n)
{
    if (s->pos >= s->len) return;
    for (uint32_t i = 0; i < n; ++i) {
        if (s->pos >= s->len) break;
        /* Envelope recurrence */
        s->env *= s->env_coef;
        float32_t noise = rng_float_mono(&s->rng);
        float32_t sample = s->env * noise * SNARE_AMP;
        L[i] += sample;
        R[i] += sample;
        if (s->env < 1e-4f) {
            s->pos = s->len; break; }
        s->pos++;
    }
}
#endif // SNARE_ASM 