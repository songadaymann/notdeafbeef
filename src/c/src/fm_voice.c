#include "fm_voice.h"
#include <math.h>
#include <stdio.h>
#include "env.h"

#define TAU 6.2831853071795864769f

void fm_voice_init(fm_voice_t *v, float32_t sr)
{
    v->sr = sr;
    v->len = 0;
    v->pos = 0;
    v->carrier_phase = 0.0f;
    v->mod_phase = 0.0f;
}

void fm_voice_trigger(fm_voice_t *v, float32_t carrier_freq, float32_t duration_sec, float32_t ratio, float32_t index, float32_t amp, float32_t decay)
{
    v->carrier_freq = carrier_freq;
    v->ratio = ratio;
    v->index0 = index;
    v->amp = amp;
    v->decay = decay;
    v->len = (uint32_t)(duration_sec * v->sr);
    v->pos = 0;
    printf("FM_TRIGGER cf=%.2f dur=%.2f ratio=%.2f idx=%.2f amp=%.2f len=%u\n", carrier_freq, duration_sec, ratio, index, amp, v->len);
}

#ifndef FM_VOICE_ASM
void fm_voice_process(fm_voice_t *v, float32_t *L, float32_t *R, uint32_t n)
{
    static int first_call=1;
    if (v->pos >= v->len) return;
    if(first_call){
        printf("FM_PROCESS first n=%u len=%u\n", n, v->len);
        first_call=0;
    }
    float32_t cp = v->carrier_phase;
    float32_t mp = v->mod_phase;
    float32_t c_inc = TAU * v->carrier_freq / v->sr;
    float32_t m_inc = TAU * v->carrier_freq * v->ratio / v->sr;
    for (uint32_t i=0;i<n;++i){
        if (v->pos >= v->len) break;
        float32_t t = (float32_t)v->pos / v->sr;
        float32_t env = env_exp_decay(t, v->decay);
        float32_t index = v->index0 * env; // optional index decay
        float32_t sample = sinf(cp + index * sinf(mp)) * env * v->amp;
        L[i]+=sample;
        R[i]+=sample;
        cp += c_inc;
        mp += m_inc;
        if(cp>=TAU) cp-=TAU;
        if(mp>=TAU) mp-=TAU;
        v->pos++;
    }
    v->carrier_phase = cp;
    v->mod_phase = mp;
}
#endif 