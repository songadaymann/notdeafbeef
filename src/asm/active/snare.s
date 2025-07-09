	.text
	.align 2
	.globl _snare_process

// Lightweight Snare – envelope recurrence + inline SplitMix64 noise
//   void snare_process(snare_t *s, float *L, float *R, uint32_t n)
//     x0 = snare_t*
//     x1 = L buffer
//     x2 = R buffer
//     w3 = n samples
//
// snare_t layout (see snare.h)
//   uint32_t pos        @ 0
//   uint32_t len        @ 4
//   float    sr         @ 8   (unused here)
//   float    env        @ 12
//   float    env_coef   @ 16
//   <padding>           @ 20
//   uint64_t rng.state  @ 24

// --- Struct Offsets ---
.equ S_POS,        0
.equ S_LEN,        4
.equ S_ENV,       12
.equ S_ENV_COEF,  16
.equ S_RNG_STATE, 24

// --- Constants ---
AMP_const:
    .float 0.4                    // overall amplitude

floats_inv24_two_one:
    .float 5.9604644775390625e-8  // 1/2^24
    .float 2.0
    .float 1.0

rng64_consts:
    .quad 0x9E3779B97F4A7C15      // GAMMA
    .quad 0xBF58476D1CE4E5B9      // MUL1
    .quad 0x94D049BB133111EB      // MUL2

// ------------------------------------------------------------
// Main routine
// ------------------------------------------------------------
_snare_process:
    // Prologue (leaf function ‑ minimal stack)
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    // Save callee-saved x22 which we use as loop counter
    str x22, [sp, #-16]!

    // Early exit if inactive or n==0
    ldr w8, [x0, #S_POS]   // pos
    ldr w9, [x0, #S_LEN]   // len
    cmp w8, w9
    b.ge Ldone             // already finished
    cbz w3, Ldone          // n == 0

    // Load mutable state
    ldr s4,  [x0, #S_ENV]        // env
    ldr s5,  [x0, #S_ENV_COEF]   // env_coef
    ldr x10, [x0, #S_RNG_STATE]  // rng.state

    // Load constants
    adrp x11, AMP_const@PAGE
    add  x11, x11, AMP_const@PAGEOFF
    ldr  s15, [x11]              // AMP

    adrp x12, floats_inv24_two_one@PAGE
    add  x12, x12, floats_inv24_two_one@PAGEOFF
    ldr  s12, [x12]              // inv24
    ldr  s13, [x12, #4]          // 2.0
    ldr  s14, [x12, #8]          // 1.0

    adrp x13, rng64_consts@PAGE
    add  x13, x13, rng64_consts@PAGEOFF

    mov w22, wzr                 // loop counter i

// ------------------------------------------------------------
Lloop:
    // Break conditions: i>=n  OR  pos>=len
    cmp w22, w3
    b.ge Lend
    cmp w8, w9
    b.ge Lend

    // env *= env_coef
    fmul s4, s4, s5

    // --- SplitMix64 ---
    ldr x14, [x13]         // GAMMA
    add x10, x10, x14      // state += GAMMA

    mov x15, x10           // z = state copy
    lsr x16, x15, #30
    eor x15, x15, x16
    ldr x16, [x13, #8]     // MUL1
    mul x15, x15, x16

    lsr x16, x15, #27
    eor x15, x15, x16
    ldr x16, [x13, #16]    // MUL2
    mul x15, x15, x16

    lsr x16, x15, #31
    eor x15, x15, x16      // final z

    // Convert to float in [-1,1)
    mov w16, w15           // low 32 bits
    lsr w16, w16, #8       // 24-bit mantissa
    ucvtf s0, w16          // to float
    fmul  s0, s0, s12      // *inv24
    fmul  s0, s0, s13      // *2
    fsub  s0, s0, s14      // -1

    // sample = env * noise * AMP
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

    // Advance indices
    add w8, w8, #1      // pos++
    add w22, w22, #1    // i++
    b   Lloop

// ------------------------------------------------------------
Lend:
    // Store updated state back to struct
    str s4,  [x0, #S_ENV]
    str w8,  [x0, #S_POS]
    str x10, [x0, #S_RNG_STATE]

Ldone:
    ldr x22, [sp], #16
    ldp x29, x30, [sp], #16
    ret 