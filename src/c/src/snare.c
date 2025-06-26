#include "snare.h"
#include <math.h>
#include "env.h"

#define SNARE_DECAY_RATE 35.0f
#define SNARE_DUR_SEC 0.1f
#define SNARE_AMP 0.4f

void snare_init(snare_t *s, float32_t sr, uint64_t seed)
{
    s->sr = sr;
    s->pos = 0;
    s->len = 0;
    s->rng = rng_seed(seed);
}

void snare_trigger(snare_t *s)
{
    s->pos = 0;
    s->len = (uint32_t)(SNARE_DUR_SEC * s->sr);
}

#ifndef SNARE_ASM
void snare_process(snare_t *s, float32_t *L, float32_t *R, uint32_t n)
{
    if (s->pos >= s->len) return;
    for (uint32_t i = 0; i < n; ++i) {
        if (s->pos >= s->len) break;
        float32_t t = (float32_t)s->pos / s->sr;
        float32_t env = env_exp_decay(t, SNARE_DECAY_RATE);
        float32_t noise = rng_float_mono(&s->rng);
        float32_t sample = env * noise * SNARE_AMP;
        L[i] += sample;
        R[i] += sample;
        s->pos++;
    }
}
#endif // SNARE_ASM 