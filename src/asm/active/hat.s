	.text
	.align 2
	.globl _hat_process

// Lightweight Hat – fast-decay envelope + white noise (SplitMix64)
//   void hat_process(hat_t *h, float *L, float *R, uint32_t n)
//     x0 = hat_t*
//     x1 = L buffer
//     x2 = R buffer
//     w3 = n samples
//
// hat_t layout (see hat.h)
//   uint32_t pos        @ 0
//   uint32_t len        @ 4
//   float    sr         @ 8 (unused)
//   float    env        @ 12
//   float    env_coef   @ 16
//   rng_t    rng        @ 24 (state 64-bit)

// --- Struct Offsets ---
.equ H_POS,        0
.equ H_LEN,        4
.equ H_ENV,       12
.equ H_ENV_COEF,  16
.equ H_RNG_STATE, 24

// --- Constants ---
H_AMP_const:
    .float 0.15                    // hat amplitude

f_inv24_two_one:
    .float 5.9604644775390625e-8   // 1/2^24
    .float 2.0
    .float 1.0

rng64_consts_hat:
    .quad 0x9E3779B97F4A7C15       // GAMMA
    .quad 0xBF58476D1CE4E5B9       // MUL1
    .quad 0x94D049BB133111EB       // MUL2

// ------------------------------------------------------------
_hat_process:
    // Prologue: minimal stack frame
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    // Preserve callee-saved x22
    str x22, [sp, #-16]!

    // Early exits
    ldr w8, [x0, #H_POS]
    ldr w9, [x0, #H_LEN]
    cmp w8, w9
    b.ge Ldone          // inactive
    cbz w3, Ldone       // n == 0

    // Load state
    ldr s4,  [x0, #H_ENV]
    ldr s5,  [x0, #H_ENV_COEF]
    ldr x10, [x0, #H_RNG_STATE]

    // Constants
    adrp x11, H_AMP_const@PAGE
    add  x11, x11, H_AMP_const@PAGEOFF
    ldr  s15, [x11]              // AMP

    adrp x12, f_inv24_two_one@PAGE
    add  x12, x12, f_inv24_two_one@PAGEOFF
    ldr  s12, [x12]
    ldr  s13, [x12, #4]
    ldr  s14, [x12, #8]

    adrp x13, rng64_consts_hat@PAGE
    add  x13, x13, rng64_consts_hat@PAGEOFF

    mov w22, wzr                 // loop counter i

// --- Main Loop ---
Lloop:
    cmp w22, w3
    b.ge Lend
    cmp w8, w9
    b.ge Lend

    // env *= env_coef
    fmul s4, s4, s5

    // SplitMix64
    ldr x14, [x13]         // GAMMA
    add x10, x10, x14
    mov x15, x10
    lsr x16, x15, #30
    eor x15, x15, x16
    ldr x16, [x13, #8]     // MUL1
    mul x15, x15, x16
    lsr x16, x15, #27
    eor x15, x15, x16
    ldr x16, [x13, #16]    // MUL2
    mul x15, x15, x16
    lsr x16, x15, #31
    eor x15, x15, x16

    mov w16, w15
    lsr w16, w16, #8
    ucvtf s0, w16
    fmul  s0, s0, s12
    fmul  s0, s0, s13
    fsub  s0, s0, s14

    // sample = env*noise*AMP
    fmul s0, s0, s4
    fmul s0, s0, s15

    // L[i] += sample
    ldr s1, [x1, w22, sxtw #2]
    fadd s1, s1, s0
    str s1, [x1, w22, sxtw #2]

    // R[i] += sample
    ldr s2, [x2, w22, sxtw #2]
    fadd s2, s2, s0
    str s2, [x2, w22, sxtw #2]

    // Advance
    add w8, w8, #1
    add w22, w22, #1
    b   Lloop

// --- Exit ---
Lend:
    str s4,  [x0, #H_ENV]
    str w8,  [x0, #H_POS]
    str x10, [x0, #H_RNG_STATE]

Ldone:
    ldr x22, [sp], #16
    ldp x29, x30, [sp], #16
    ret 