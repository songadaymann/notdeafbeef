// fast_math_neon.h – 4-lane NEON approximations for expf() and sinf()
// Source: adapted from Julien Pommier's "neon_mathfun" (zlib licence)
// Provides ~1 ulp accuracy, vastly faster than libm scalar calls.
#pragma once

#ifdef __ARM_NEON
#include <arm_neon.h>

// ------- exp4_ps -----------------------------------------------------------
static inline float32x4_t exp4_ps(float32x4_t x)
{
    const float32x4_t max_x = vdupq_n_f32( 88.3762626647949f);
    const float32x4_t min_x = vdupq_n_f32(-88.3762626647949f);
    x = vmaxq_f32(vminq_f32(x, max_x), min_x);

    const float32x4_t log2e  = vdupq_n_f32(1.44269504088896341f);
    const float32x4_t half   = vdupq_n_f32(0.5f);
    float32x4_t fx = vmlaq_f32(half, x, log2e);
    int32x4_t   emm0 = vcvtq_s32_f32(fx);
    fx = vcvtq_f32_s32(emm0);

    const float32x4_t ln2_hi = vdupq_n_f32(0.693359375f);
    const float32x4_t ln2_lo = vdupq_n_f32(-2.12194440e-4f);
    float32x4_t tmp = vmulq_f32(fx, ln2_hi);
    float32x4_t z   = vmulq_f32(fx, ln2_lo);
    x = vsubq_f32(x, tmp);
    x = vsubq_f32(x, z);

    const float32x4_t c1 = vdupq_n_f32(1.9875691500E-4f);
    const float32x4_t c2 = vdupq_n_f32(1.3981999507E-3f);
    const float32x4_t c3 = vdupq_n_f32(8.3334519073E-3f);
    const float32x4_t c4 = vdupq_n_f32(4.1665795894E-2f);
    const float32x4_t c5 = vdupq_n_f32(1.6666665459E-1f);

    float32x4_t x2 = vmulq_f32(x, x);

    float32x4_t y = vmlaq_f32(c1, x, c2);
    y = vmlaq_f32(y, x2, c3);
    y = vmlaq_f32(y, vmulq_f32(x2, x), c4);
    y = vmlaq_f32(y, vmulq_f32(x2, x2), c5);

    y = vaddq_f32(vaddq_f32(y, x), vdupq_n_f32(1.0f));

    emm0 = vaddq_s32(emm0, vdupq_n_s32(127));
    emm0 = vshlq_n_s32(emm0, 23);
    float32x4_t pow2n = vreinterpretq_f32_s32(emm0);

    return vmulq_f32(y, pow2n);
}

// ------- sin4_ps -----------------------------------------------------------
static inline float32x4_t sin4_ps(float32x4_t x)
{
    // Range reduce to [-π, π]
    const float32x4_t inv_pi = vdupq_n_f32(0.31830988618379067154f); // 1/π
    const float32x4_t pi     = vdupq_n_f32(3.14159265358979323846f);

    float32x4_t y = vmulq_f32(x, inv_pi);
    int32x4_t n = vcvtq_s32_f32(vrndnq_f32(y));
    y = vcvtq_f32_s32(n);
    x = vsubq_f32(x, vmulq_f32(y, pi));

    // flip sign if n is odd
    uint32x4_t swap_sign = vandq_u32(vreinterpretq_u32_s32(n), vdupq_n_u32(1));
    x = vbslq_f32(vshlq_n_u32(swap_sign,31), vnegq_f32(x), x);

    const float32x4_t s1 = vdupq_n_f32(-1.6666664611e-1f);
    const float32x4_t s2 = vdupq_n_f32( 8.3333337670e-3f);
    const float32x4_t s3 = vdupq_n_f32(-1.9841270114e-4f);
    const float32x4_t s4 = vdupq_n_f32( 2.7557314297e-6f);

    float32x4_t z = vmulq_f32(x, x);

    float32x4_t y1 = vmlaq_f32(s2, z, s3);
    y1 = vmlaq_f32(y1, vmulq_f32(z,z), s4);
    y1 = vmlaq_f32(y1, z, s1);

    return vmlaq_f32(x, vmulq_f32(x, vmulq_f32(z, y1)), vdupq_n_f32(0.0f));
}

#endif // __ARM_NEON 