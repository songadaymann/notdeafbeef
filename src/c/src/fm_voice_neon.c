#ifdef float32_t
#undef float32_t
#endif
#ifdef __ARM_NEON
#include <arm_neon.h>
#endif
#include <math.h>
#include <stdint.h>
#include "fm_voice.h"
#include "fast_math_neon.h"
#include "fast_math_asm.h"

#ifndef TAU
#define TAU 6.2831853071795864769f
#endif

#ifdef __ARM_NEON
#define EXP4 exp4_ps_asm
#define SIN4 sin4_ps_asm
#else
#define EXP4 exp4_ps
#define SIN4 sin4_ps
#endif

// Road-map Phase 4.2b — 4-sample NEON implementation that still relies on
// scalar sinf/expf for transcendental ops.  It delivers functional parity
// while laying the groundwork for 4.2c where we replace those calls with
// polynomial/vector approximations.

void fm_voice_process(fm_voice_t *v, float32_t *L, float32_t *R, uint32_t n)
{
    if (!v || n==0) return;
    const float32_t sr=v->sr;
    const float32_t carrier_freq=v->carrier_freq;
    const float32_t ratio=v->ratio;
    const float32_t index0=v->index0;
    const float32_t amp=v->amp;
    const float32_t decay=v->decay;
    float32_t cp=v->carrier_phase;
    float32_t mp=v->mod_phase;
    const float32_t c_inc = TAU * carrier_freq / sr;
    const float32_t m_inc = TAU * carrier_freq * ratio / sr;
    uint32_t i=0;
#ifdef __ARM_NEON
    if (v->pos < v->len) {
        for (; i + 3 < n && v->pos + 3 < v->len; i += 4) {
            const float32x4_t lane = {0.f,1.f,2.f,3.f};

            /* phase vectors */
            float32x4_t cp_v = vaddq_f32(vdupq_n_f32(cp), vmulq_n_f32(lane, c_inc));
            float32x4_t mp_v = vaddq_f32(vdupq_n_f32(mp), vmulq_n_f32(lane, m_inc));

            /* time vector for envelope */
            float32x4_t pos_v = { (float)(v->pos+0), (float)(v->pos+1),
                                  (float)(v->pos+2), (float)(v->pos+3) };
            pos_v = vmulq_n_f32(pos_v, 1.0f / sr);

            /* env = exp(-decay * t) */
            float32x4_t env_v = EXP4(vmulq_n_f32(pos_v, -decay));

            /* index & mod */
            float32x4_t idx_v   = vmulq_n_f32(env_v, index0);
            float32x4_t inner_v = SIN4(mp_v);

            float32x4_t arg_v   = vmlaq_f32(cp_v, idx_v, inner_v);
            float32x4_t sample_v= SIN4(arg_v);

            sample_v = vmulq_f32(sample_v, env_v);
            sample_v = vmulq_n_f32(sample_v, amp);

            /* accumulate */
            vst1q_f32(L+i, vaddq_f32(vld1q_f32(L+i), sample_v));
            vst1q_f32(R+i, vaddq_f32(vld1q_f32(R+i), sample_v));

            /* advance phases / pos */
            cp += 4*c_inc; if(cp>=TAU) cp=fmodf(cp,TAU);
            mp += 4*m_inc; if(mp>=TAU) mp=fmodf(mp,TAU);
            v->pos += 4;
        }
    }
#endif // __ARM_NEON
    // scalar loop (may run all or tail)
    for(; i<n && v->pos<v->len; ++i){
        float32_t t=(float32_t)v->pos/sr;
        float32_t env=expf(-decay*t);
        float32_t idx=index0*env;
        float32_t sample=sinf(cp + idx * sinf(mp))*env*amp;
        L[i]+=sample;
        R[i]+=sample;
        cp+=c_inc; if(cp>=TAU) cp-=TAU;
        mp+=m_inc; if(mp>=TAU) mp-=TAU;
        v->pos++;
    }
    v->carrier_phase=cp;
    v->mod_phase=mp;
} 