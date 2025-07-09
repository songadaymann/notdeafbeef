	.text
	.align 2
	.globl _kick_process

// --- Constants ---
AMP_const:
    .float 0.8            // overall amplitude

// Offsets inside kick_t struct (see kick.h)
.equ K_SR,         0     // float sr
.equ K_POS,        4     // uint32 pos
.equ K_LEN,        8     // uint32 len
.equ K_ENV,        12    // float env
.equ K_ENV_COEF,   16    // float env_coef
.equ K_Y_PREV,     28    // float y_prev (sin(theta[n-1]))
.equ K_Y_PREV2,    32    // float y_prev2 (sin(theta[n-2]))
.equ K_K1,         36    // float k1 = 2*cos(delta)

// void kick_process(kick_t *k, float *L, float *R, uint32_t n)
// x0 = kick*, x1 = L*, x2 = R*, w3 = n
_kick_process:
    // Prologue (minimal – leaf function, no lib calls)
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    // Preserve callee-saved x22 (used as loop counter)
    str x22, [sp, #-16]!

    // early-out: inactive or n==0
    ldr w9, [x0, #K_POS]
    ldr w10,[x0, #K_LEN]
    cmp w9, w10
    b.ge Ldone          // pos >= len
    cbz w3, Ldone       // n==0

    // Load state into FP regs
    ldr s4,  [x0, #K_ENV]       // env
    ldr s5,  [x0, #K_ENV_COEF]  // env_coef
    ldr s6,  [x0, #K_Y_PREV]    // y_prev
    ldr s7,  [x0, #K_Y_PREV2]   // y_prev2
    ldr s8,  [x0, #K_K1]        // k1 = 2*cos(delta)

    // Load AMP constant once
    adrp x11, AMP_const@PAGE
    add  x11, x11, AMP_const@PAGEOFF
    ldr  s15, [x11]

    mov w22, wzr                // i counter

Lloop:
    // Check end conditions
    cmp w22, w3
    b.ge Lend
    cmp w9, w10
    b.ge Lend

    // env *= env_coef
    fmul s4, s4, s5

    // y = k1*y_prev - y_prev2  (use s9 temps)
    fmul s9, s8, s6     // k1*y_prev
    fsub s9, s9, s7     // - y_prev2 -> y

    // sample = env * y * AMP
    fmul s0, s4, s9
    fmul s0, s0, s15

    // L[i] += sample
    ldr s1, [x1, w22, sxtw #2]
    fadd s1, s1, s0
    str s1, [x1, w22, sxtw #2]

    // R[i] += sample
    ldr s2, [x2, w22, sxtw #2]
    fadd s2, s2, s0
    str s2, [x2, w22, sxtw #2]

    // Update sine recurrence state
    fmov s7, s6         // y_prev2 = old y_prev
    fmov s6, s9         // y_prev  = y

    // Increment counters
    add w9, w9, #1      // pos++
    add w22, w22, #1    // i++
    b   Lloop

Lend:
    // Store back updated state
    str s4,  [x0, #K_ENV]
    str s6,  [x0, #K_Y_PREV]
    str s7,  [x0, #K_Y_PREV2]
    str w9,  [x0, #K_POS]

Ldone:
    ldr x22, [sp], #16
    ldp x29, x30, [sp], #16
    ret 