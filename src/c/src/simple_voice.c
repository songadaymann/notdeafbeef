#include "simple_voice.h"
#include <math.h>
#include "env.h"

#define TAU 6.2831853071795864769f

void simple_voice_init(simple_voice_t *v, float32_t sr)
{
    osc_reset(&v->osc);
    v->sr = sr;
    v->len = 0;
    v->pos = 0;
    v->decay = 6.0f;
    v->amp = 0.2f;
    v->wave = SIMPLE_SINE;
    v->freq = 440.0f;
}

void simple_voice_trigger(simple_voice_t *v, float32_t freq, float32_t dur_sec, simple_wave_t wave, float32_t amp, float32_t decay)
{
    v->freq = freq;
    v->wave = wave;
    v->amp  = amp;
    v->decay = decay;
    v->len = (uint32_t)(dur_sec * v->sr);
    v->pos = 0;
}

static inline float32_t render_sample(simple_voice_t *v)
{
    float32_t phase = v->osc.phase;
    float32_t frac = phase / TAU; /* 0..1 */
    float32_t s=0.0f;
    switch(v->wave){
        case SIMPLE_TRI:
            s = 2.0f * fabsf(2.0f * frac - 1.0f) - 1.0f;
            break;
        case SIMPLE_SQUARE:
            s = (frac < 0.5f) ? 1.0f : -1.0f;
            break;
        default: /* sine */
            s = sinf(phase);
            break;
    }
    return s;
}

void simple_voice_process(simple_voice_t *v, float32_t *L, float32_t *R, uint32_t n)
{
    if(v->pos >= v->len) return;
    float32_t phase = v->osc.phase;
    float32_t inc = TAU * v->freq / v->sr;
    for(uint32_t i=0;i<n;++i){
        if(v->pos >= v->len) break;
        float32_t t = (float32_t)v->pos / v->sr;
        float32_t env = env_exp_decay(t, v->decay);
        float32_t sample;
        float32_t frac = phase / TAU;
        switch(v->wave){
            case SIMPLE_TRI:
                sample = (2.0f * fabsf(2.0f * frac - 1.0f) - 1.0f);
                break;
            case SIMPLE_SQUARE:
                sample = (frac < 0.5f)?1.0f:-1.0f;
                break;
            default:
                sample = sinf(phase);
                break;
        }
        sample *= env * v->amp;
        L[i]+=sample;
        R[i]+=sample;
        phase += inc;
        if(phase>=TAU) phase -= TAU;
        v->pos++;
    }
    v->osc.phase = phase;
} 