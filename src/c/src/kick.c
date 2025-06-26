#include "kick.h"
#include <math.h>
#include "env.h"

#define TAU 6.28318530717958647692f

/* Kick parameters */
#define KICK_BASE_FREQ 50.0f
#define KICK_DECAY_RATE 20.0f   /* amplitude envelope exp rate */
#define KICK_MAX_LEN_SEC 1.0f   /* max duration */
#define KICK_AMP 0.8f

void kick_init(kick_t *k, float32_t sr)
{
    k->sr = sr;
    k->pos = 0;
    k->len = 0;
}

void kick_trigger(kick_t *k)
{
    k->pos = 0;
    k->len = (uint32_t)(KICK_MAX_LEN_SEC * k->sr);
}

#ifndef KICK_ASM
void kick_process(kick_t *k, float32_t *L, float32_t *R, uint32_t n)
{
    if (k->pos >= k->len) return; // inactive

    for (uint32_t i = 0; i < n; ++i) {
        if (k->pos >= k->len) break;
        float32_t t = (float32_t)k->pos / k->sr;           /* time in seconds */
        float32_t env = env_exp_decay(t, KICK_DECAY_RATE);
        float32_t tone = sinf(TAU * KICK_BASE_FREQ * t);
        float32_t sample = env * tone * KICK_AMP;
        L[i] += sample;
        R[i] += sample;
        k->pos++;
    }
    /* if we ended early, remaining n-i samples no addition, fine */
}
#endif 