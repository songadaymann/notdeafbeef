#include "osc.h"
#include <math.h>

#define TAU 6.28318530717958647692f

#ifndef OSC_SINE_ASM
void osc_sine_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr)
{
    const float32_t phase_inc = TAU * freq / sr;
    float32_t ph = o->phase;
    for (uint32_t i = 0; i < n; ++i) {
        out[i] = sinf(ph);
        ph += phase_inc;
        if (ph >= TAU) ph -= TAU;
    }
    o->phase = ph;
}
#endif

#ifndef OSC_SHAPES_ASM
void osc_saw_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr)
{
    const float32_t phase_inc = TAU * freq / sr;
    float32_t ph = o->phase;
    for (uint32_t i = 0; i < n; ++i) {
        float32_t frac = ph / TAU; // 0..1
        out[i] = 2.0f * frac - 1.0f; // -1..1
        ph += phase_inc;
        if (ph >= TAU) ph -= TAU;
    }
    o->phase = ph;
}

void osc_square_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr)
{
    const float32_t phase_inc = TAU * freq / sr;
    float32_t ph = o->phase;
    for (uint32_t i = 0; i < n; ++i) {
        out[i] = (ph < TAU * 0.5f) ? 1.0f : -1.0f;
        ph += phase_inc;
        if (ph >= TAU) ph -= TAU;
    }
    o->phase = ph;
}

void osc_triangle_block(osc_t *o, float32_t *out, uint32_t n, float32_t freq, float32_t sr)
{
    const float32_t phase_inc = TAU * freq / sr;
    float32_t ph = o->phase;
    for (uint32_t i = 0; i < n; ++i) {
        float32_t frac = ph / TAU; // 0..1
        float32_t val = 2.0f * fabsf(2.0f * frac - 1.0f) - 1.0f; // triangle -1..1
        out[i] = val;
        ph += phase_inc;
        if (ph >= TAU) ph -= TAU;
    }
    o->phase = ph;
}
#endif 