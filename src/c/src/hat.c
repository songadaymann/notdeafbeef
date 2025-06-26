#include "hat.h"
#include <math.h>
#include "env.h"

#define HAT_DECAY_RATE 120.0f
#define HAT_DUR_SEC 0.05f
#define HAT_AMP 0.15f

void hat_init(hat_t *h, float32_t sr, uint64_t seed)
{
    h->pos=0; h->len=0; h->sr=sr;
    h->rng = rng_seed(seed);
}

void hat_trigger(hat_t *h)
{
    h->pos = 0;
    h->len = (uint32_t)(HAT_DUR_SEC * h->sr);
}

#ifndef HAT_ASM
void hat_process(hat_t *h, float32_t *L, float32_t *R, uint32_t n)
{
    if (h->pos >= h->len) return;
    for(uint32_t i=0;i<n;++i){
        if(h->pos >= h->len) break;
        float32_t t = (float32_t)h->pos / h->sr;
        float32_t env = env_exp_decay(t, HAT_DECAY_RATE);
        float32_t noise = rng_float_mono(&h->rng);
        float32_t sample = noise * env * HAT_AMP;
        L[i] += sample;
        R[i] += sample;
        h->pos++;
    }
}
#endif // HAT_ASM 