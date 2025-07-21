.text
.align 2
.globl _sin4_ps_asm
.private_extern sin4_ps_asm_internal
sin4_ps_asm_internal:
_sin4_ps_asm:
    // v0 contains input x; will carry final result.
    // Load constant table base
    adrp x9, Lsin_const@PAGE
    add  x9, x9, Lsin_const@PAGEOFF

    // y = x * inv_pi
    ldr  q1, [x9, #0]      // inv_pi
    fmul v1.4s, v0.4s, v1.4s   // v1 = y (float)

    // Round y to nearest even integer
    frintn v2.4s, v1.4s        // v2 = rounded float
    fcvtzs v3.4s, v2.4s        // v3 = int32 n
    // Convert back to float
    scvtf v4.4s, v3.4s         // v4 = y as float

    // x = x - y * pi
    ldr  q5, [x9, #16]     // pi
    fmul v6.4s, v4.4s, v5.4s
    fsub v0.4s, v0.4s, v6.4s

    // swap_sign = n & 1
    movi v7.4s, #1
    and  v8.16b, v3.16b, v7.16b   // v8 holds 0 or 1
    shl  v8.4s, v8.4s, #31        // move to sign bit position

    // Toggle sign bit where needed
    eor  v0.16b, v0.16b, v8.16b

    // Polynomial evaluation
    // z = x*x
    fmul v9.4s, v0.4s, v0.4s   // z

    ldr  q10, [x9, #32]    // s2
    ldr  q11, [x9, #48]    // s3
    ldr  q12, [x9, #64]    // s4
    ldr  q13, [x9, #80]    // s1

    // y1 = s2 + z*s3
    mov  v14.16b, v10.16b       // y1 = s2
    fmla v14.4s, v9.4s, v11.4s  // + z*s3

    // z2 = z*z
    fmul v15.4s, v9.4s, v9.4s   // z2
    fmla v14.4s, v15.4s, v12.4s // + z2*s4

    // y1 += z * s1
    fmla v14.4s, v9.4s, v13.4s

    // w = z * y1
    fmul v16.4s, v9.4s, v14.4s
    // x += x * w
    fmla v0.4s, v0.4s, v16.4s

    ret

    // Constant table (16-byte aligned)
    .align 4
Lsin_const:
    // inv_pi (1/π)
    .float 0.31830988618379067154, 0.31830988618379067154, 0.31830988618379067154, 0.31830988618379067154
    // pi
    .float 3.14159265358979323846, 3.14159265358979323846, 3.14159265358979323846, 3.14159265358979323846
    // s2
    .float 8.3333337670e-3, 8.3333337670e-3, 8.3333337670e-3, 8.3333337670e-3
    // s3
    .float -1.9841270114e-4, -1.9841270114e-4, -1.9841270114e-4, -1.9841270114e-4
    // s4
    .float 2.7557314297e-6, 2.7557314297e-6, 2.7557314297e-6, 2.7557314297e-6
    // s1
    .float -1.6666664611e-1, -1.6666664611e-1, -1.6666664611e-1, -1.6666664611e-1 