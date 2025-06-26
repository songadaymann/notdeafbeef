#include "melody.h"
#include <math.h>
#include "env.h"

#define TAU 6.2831853071795864769f
#define MELODY_DECAY_RATE 5.0f
#define MELODY_MAX_SEC 2.0f

void melody_init(melody_t *m, float32_t sr)
{
    osc_reset(&m->osc);
    m->sr = sr;
    m->len = 0;
    m->pos = 0;
    m->freq = 440.0f;
}

void melody_trigger(melody_t *m, float32_t freq, float32_t dur_sec)
{
    m->freq = freq;
    m->len = (uint32_t)(dur_sec * m->sr);
    if(m->len > (uint32_t)(MELODY_MAX_SEC * m->sr))
        m->len = (uint32_t)(MELODY_MAX_SEC * m->sr);
    m->pos = 0;
}

#ifndef MELODY_ASM
void melody_process(melody_t *m, float32_t *L, float32_t *R, uint32_t n)
{
    if (m->pos >= m->len) return;
    float32_t phase = m->osc.phase;
    float32_t phase_inc = TAU * m->freq / m->sr;
    for(uint32_t i=0;i<n;++i){
        if(m->pos >= m->len) break;
        float32_t frac = phase / TAU;
        /* raw sawtooth */
        float32_t raw = 2.0f * frac - 1.0f;
        /* driven & soft-clipped (1.5x − 0.5x³) to match python timbre */
        float32_t driven = 1.2f * raw;
        float32_t soft = 1.5f * driven - 0.5f * driven * driven * driven;
        float32_t saw = soft;
        float32_t t = (float32_t)m->pos / m->sr;
        float32_t env = env_exp_decay(t, MELODY_DECAY_RATE);
#ifdef NO_MID_FM
        float32_t sample = saw * env * 0.15f;
#else
        float32_t sample = saw * env * 0.25f; // scale down
#endif
        L[i]+=sample;
        R[i]+=sample;
        phase += phase_inc;
        if(phase >= TAU) phase -= TAU;
        m->pos++;
    }
    m->osc.phase = phase;
}
#endif // MELODY_ASM 