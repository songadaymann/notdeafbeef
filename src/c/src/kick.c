#include "kick.h"
#include <math.h>
#include "env.h"

#define TAU 6.28318530717958647692f
#define KICK_LUT_LEN 1024

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
    k->env = 0.0f;
    k->env_coef = 0.0f;
    k->phase = 0.0f;
    k->phase_inc = (float32_t)KICK_LUT_LEN * KICK_BASE_FREQ / k->sr;
    k->y_prev = 0.0f;
    k->y_prev2 = 0.0f;
    k->k1 = 0.0f;
    float32_t delta = TAU * KICK_BASE_FREQ / k->sr;
    float32_t sin_delta = sinf(delta);
    float32_t cos_delta = cosf(delta);
    k->k1 = 2.0f * cos_delta;
    k->y_prev2 = -sin_delta; // y[-1]
}

void kick_trigger(kick_t *k)
{
    k->pos = 0;
    k->len = (uint32_t)(KICK_MAX_LEN_SEC * k->sr);
    k->env = 1.0f;
    k->env_coef = expf(-KICK_DECAY_RATE / k->sr);
    k->phase = 0.0f;

    /* Reset sine recurrence state */
    float32_t delta = TAU * KICK_BASE_FREQ / k->sr;
    k->y_prev  = 0.0f;            /* sin(0) */
    k->y_prev2 = -sinf(delta);    /* y[-1] = -sin(Δ) */
}

#ifndef KICK_ASM
void kick_process(kick_t *k, float32_t *L, float32_t *R, uint32_t n)
{
    if (k->pos >= k->len) return; // inactive

    for (uint32_t i = 0; i < n; ++i) {
        if (k->pos >= k->len) break;

        /* Per-sample envelope using recurrence */
        k->env *= k->env_coef;

        /* Two-tap sine recurrence */
        float32_t y = k->k1 * k->y_prev - k->y_prev2;
        k->y_prev2 = k->y_prev;
        k->y_prev  = y;

        float32_t sample = k->env * y * KICK_AMP;
        L[i] += sample;
        R[i] += sample;

        if (k->env < 1e-4f) {
            /* Envelope effectively silent – stop early */
            k->pos = k->len;
            break;
        }
        k->pos++;
    }
}
#endif 