.text
.align 2
.globl _exp4_ps_asm

// float32x4_t exp4_ps_asm(float32x4_t x)
// 4-wide single-precision e^x approximation, ported from fast_math_neon.h.
// Identical maths to the C intrinsics version.

_exp4_ps_asm:
    // v0 holds input vector x and will carry the final result.

    // Load constant table base
    adrp x9, Lexp_const@PAGE
    add  x9, x9, Lexp_const@PAGEOFF

    // Clamp x to [min_x, max_x]
    ldr q1,  [x9, #0]      // max_x
    ldr q2,  [x9, #16]     // min_x
    fmin v3.4s, v0.4s, v1.4s   // v3 = min(x,max_x)
    fmax v0.4s, v3.4s, v2.4s   // x = max(v3,min_x)

    // fx = x * log2e + 0.5
    ldr q4,  [x9, #32]     // log2e
    fmul v5.4s, v0.4s, v4.4s
    ldr q6,  [x9, #48]     // 0.5
    fadd v5.4s, v5.4s, v6.4s

    // Convert to int (truncate toward zero)
    fcvtzs v7.4s, v5.4s         // emm0

    // fx = float(emm0)
    scvtf v8.4s, v7.4s

    // x -= fx * ln2_hi + fx * ln2_lo
    ldr q9,  [x9, #64]     // ln2_hi
    ldr q10, [x9, #80]     // ln2_lo
    fmul v11.4s, v8.4s, v9.4s
    fmul v12.4s, v8.4s, v10.4s
    fsub v0.4s, v0.4s, v11.4s
    fsub v0.4s, v0.4s, v12.4s

    // Polynomial approximation
    fmul v13.4s, v0.4s, v0.4s   // x2 = x*x

    ldr q14, [x9, #96]     // c1
    ldr q15, [x9, #112]    // c2
    fmla v14.4s, v0.4s, v15.4s

    ldr q16, [x9, #128]    // c3
    fmla v14.4s, v13.4s, v16.4s

    fmul v17.4s, v13.4s, v0.4s  // x2*x
    ldr q18, [x9, #144]    // c4
    fmla v14.4s, v17.4s, v18.4s

    fmul v19.4s, v13.4s, v13.4s // x2*x2
    ldr q20, [x9, #160]    // c5
    fmla v14.4s, v19.4s, v20.4s

    // y += x
    fadd v14.4s, v14.4s, v0.4s

    // y += 1.0f
    ldr q21, [x9, #176]    // 1.0f vector
    fadd v14.4s, v14.4s, v21.4s

    // construct 2^n
    movi v22.4s, #127           // 127
    add  v7.4s, v7.4s, v22.4s
    shl  v7.4s, v7.4s, #23
    mov  v23.16b, v7.16b

    // result = y * 2^n
    fmul v0.4s, v14.4s, v23.4s
    ret

    .align 4
Lexp_const:
    // max_x
    .float 88.3762626647949, 88.3762626647949, 88.3762626647949, 88.3762626647949
    // min_x
    .float -88.3762626647949, -88.3762626647949, -88.3762626647949, -88.3762626647949
    // log2e
    .float 1.44269504088896341, 1.44269504088896341, 1.44269504088896341, 1.44269504088896341
    // 0.5
    .float 0.5, 0.5, 0.5, 0.5
    // ln2_hi
    .float 0.693359375, 0.693359375, 0.693359375, 0.693359375
    // ln2_lo
    .float -2.12194440e-4, -2.12194440e-4, -2.12194440e-4, -2.12194440e-4
    // c1
    .float 1.9875691500E-4, 1.9875691500E-4, 1.9875691500E-4, 1.9875691500E-4
    // c2
    .float 1.3981999507E-3, 1.3981999507E-3, 1.3981999507E-3, 1.3981999507E-3
    // c3
    .float 8.3334519073E-3, 8.3334519073E-3, 8.3334519073E-3, 8.3334519073E-3
    // c4
    .float 4.1665795894E-2, 4.1665795894E-2, 4.1665795894E-2, 4.1665795894E-2
    // c5
    .float 0.16666665459, 0.16666665459, 0.16666665459, 0.16666665459
    // 1.0
    .float 1.0, 1.0, 1.0, 1.0 