	.text
	.align 2
	.globl _melody_process

// ----------------------------------------------------------------------------
// void melody_process(melody_t *m, float *L, float *R, uint32_t n)
//   x0 = melody_t*
//   x1 = L buffer
//   x2 = R buffer
//   w3 = n
// ----------------------------------------------------------------------------
// Struct layout (offsets)
// 0   : osc.phase (float)
// 4   : pos (uint32)
// 8   : len (uint32)
// 12  : sr (float)
// 16  : freq (float)
// ----------------------------------------------------------------------------
// Constants table (floats unless noted)
melody_f32_consts:
    .float 6.2831853071795864769      // [0] TAU
    .float 0.15915494309189533577     // [4] 1/TAU
    .float 5.0                        // [8] DECAY_RATE
    .float 0.25                       // [12] AMP
    .float 2.0                        // [16] TWO
    .float 1.0                        // [20] ONE
    .float 1.2                        // [24] DRIVE_GAIN
    .float 1.5                        // [28] SOFT_A
    .float 0.5                        // [32] SOFT_B

_melody_process:
    // Prologue
    stp x29, x30, [sp, #-288]!
    stp q8,  q9,  [sp, #112]
    stp q10, q11, [sp, #144]
    stp q12, q13, [sp, #176]
    stp q14, q15, [sp, #208]
    mov x29, sp
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    stp x25, x26, [sp, #64]
    // SAFE OFFSETS: x27, x28 stored at safer location to avoid conflicts
    stp x27, x28, [sp, #96]

    // Save commonly used pointers
    mov x19, x0      // melody*
    mov x23, x1      // L*
    mov x24, x2      // R*
    mov w22, w3      // n

    // Load struct members
    ldr s16, [x19]        // phase (s16)
    ldr w20, [x19, #4]    // pos (w20)
    ldr w21, [x19, #8]    // len (w21)
    ldr s17, [x19, #12]   // sr (s17)
    ldr s18, [x19, #16]   // freq (s18)

    // Early-outs
    cmp w20, w21
    b.ge Ldone
    cbz w22, Ldone

    // Load constants pointer
    adrp x27, melody_f32_consts@PAGE
    add  x27, x27, melody_f32_consts@PAGEOFF
    // Load constants
    ldr  s8,  [x27]        // TAU
    ldr  s9,  [x27, #4]    // INV_TAU
    ldr  s10, [x27, #8]    // DECAY_RATE
    ldr  s11, [x27, #12]   // AMP
    ldr  s12, [x27, #16]   // 2.0
    ldr  s13, [x27, #20]   // 1.0
    ldr  s14, [x27, #24]   // DRIVE_GAIN 1.2
    ldr  s15, [x27, #28]   // SOFT_A 1.5
    ldr  s3,  [x27, #32]   // SOFT_B 0.5

    // Reload sr and freq after libm call and recompute phase_inc
    ldr  s17, [x19, #12]   // sr
    ldr  s18, [x19, #16]   // freq
    fmul s30, s8, s18      // TAU*freq
    fdiv s30, s30, s17     // /sr

    mov w28, wzr           // loop counter i

Lloop:
    // Exit conditions
    cmp w28, w22
    b.ge Lend
    cmp w20, w21
    b.ge Lend

    // t = pos / sr
    ucvtf s0, w20
    fdiv  s0, s0, s17      // t

    // env = expf(-DECAY * t)
    fmul s1, s10, s0       // decay*t
    fneg s1, s1            // -decay*t
    fmov s0, s1
    // BUFFER POINTER PRESERVATION: Save x23/x24 to safe slots before libm call
    str x23, [x29, #272]
    str x24, [x29, #280]
    // Save registers before libm call
    sub sp, sp, #128       // Scratch frame (128-byte) prevents overlap with main frame slots
    stp x0, x1, [sp]
    stp x2, x3, [sp, #8]
    stp x19, x20, [sp, #16]
    stp s8,  s9,  [sp, #24]
    stp s10, s11, [sp, #32]
    stp s12, s13, [sp, #40]
    stp s14, s15, [sp, #48]
    stp s16, s17, [sp, #56]
    stp s18, s19, [sp, #64]
    stp s20, s21, [sp, #72]
    stp x23, x24, [sp, #-16]!   // Preserve L/R pointers across libm
    bl   _expf           // env in s0
    ldp x23, x24, [sp], #16
    // Restore registers after libm call
    ldp s20, s21, [sp, #72]
    ldp s18, s19, [sp, #64]
    ldp s16, s17, [sp, #56]
    ldp s14, s15, [sp, #48]
    ldp s12, s13, [sp, #40]
    ldp s10, s11, [sp, #32]
    ldp s8,  s9,  [sp, #24]
    ldp x19, x20, [sp, #16]
    ldp x2,  x3,  [sp, #8]
    ldp x0,  x1,  [sp]
    add sp, sp, #128
    // BUFFER POINTER PRESERVATION: Restore x23/x24 from safe slots after libm call
    ldr x23, [x29, #272]
    ldr x24, [x29, #280]
    // Restore phase is unnecessary because s16 was preserved in the scratch frame
    fmov s4, s0            // env in s4

    // Reload SOFT_B (0.5) which lives in caller-saved v3 that expf may clobber
    ldr  s3, [x27, #32]

    // frac = phase / TAU = phase * INV_TAU
    fdiv s5, s16, s8       // frac

    // raw = 2*frac -1
    fmul s6, s5, s12       // 2*frac
    fsub s6, s6, s13       // -1 -> raw in s6

    // driven = 1.2 * raw
    fmul s7, s14, s6       // driven

    // driven^3
    fmul s0, s7, s7        // driven^2
    fmul s0, s0, s7        // driven^3 in s0

    // soft = 1.5*driven - 0.5*driven^3
    fmul s1, s15, s7       // 1.5*driven
    fmul s2, s3,  s0       // 0.5*driven^3
    fsub s5, s1, s2        // soft in s5

    // sample = soft * env * AMP (s11)
    fmul s5, s5, s4
    fmul s5, s5, s11

    // Add to L[i]
    ldr s0, [x23, w28, sxtw #2]
    fadd s0, s0, s5
    str s0, [x23, w28, sxtw #2]

    // Add to R[i]
    ldr s1, [x24, w28, sxtw #2]
    fadd s1, s1, s5
    str s1, [x24, w28, sxtw #2]

    // Increment counters & position
    add w20, w20, #1       // pos++
    add w28, w28, #1       // i++
    // Advance phase using computed increment (s30)
    fadd s16, s16, s30
    // Wrap after increment as above
    fcmp s16, s8
    b.lt Lcontinue
    fsub s16, s16, s8
Lcontinue:

    b   Lloop

Lend:
    // Store back state
    str s16, [x19]      // phase
    str w20, [x19, #4]  // pos

Ldone:
    // Epilogue
    ldp x27, x28, [sp, #96]
    ldp x25, x26, [sp, #64]
    ldp x23, x24, [sp, #48]
    ldp x21, x22, [sp, #32]
    ldp x19, x20, [sp, #16]
    ldp q14, q15, [sp, #208]
    ldp q12, q13, [sp, #176]
    ldp q10, q11, [sp, #144]
    ldp q8,  q9,  [sp, #112]
    ldp x29, x30, [sp], #288
    dmb ish    // Ensure memory writes visible before returning
    ret 