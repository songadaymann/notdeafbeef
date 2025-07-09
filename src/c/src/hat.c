#include "hat.h"
#include <math.h>

#define HAT_DECAY_RATE 120.0f
#define HAT_DUR_SEC 0.05f
#define HAT_AMP 0.15f

void hat_init(hat_t *h, float32_t sr, uint64_t seed)
{
    h->pos=0; h->len=0; h->sr=sr;
    h->env = 0.0f;
    h->env_coef = 0.0f;
    h->rng = rng_seed(seed);
}

void hat_trigger(hat_t *h)
{
    h->pos = 0;
    h->len = (uint32_t)(HAT_DUR_SEC * h->sr);
    h->env = 1.0f;
    h->env_coef = expf(-HAT_DECAY_RATE / h->sr);
}

#ifndef HAT_ASM
void hat_process(hat_t *h, float32_t *L, float32_t *R, uint32_t n)
{
    if (h->pos >= h->len) return;
    for(uint32_t i=0;i<n;++i){
        if(h->pos >= h->len) break;
        h->env *= h->env_coef;
        float32_t noise = rng_float_mono(&h->rng);
        float32_t sample = noise * h->env * HAT_AMP;
        L[i] += sample;
        R[i] += sample;
        if (h->env < 1e-5f) { h->pos = h->len; break; }
        h->pos++;
    }
}
#endif // HAT_ASM 