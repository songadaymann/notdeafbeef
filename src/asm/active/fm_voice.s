.text
.align 2
.globl _fm_voice_process

// void fm_voice_process(fm_voice_t *v, float *L, float *R, uint32_t n)
// Args:
//   x0 = fm_voice_t* (state struct)
//   x1 = L buffer
//   x2 = R buffer
//   w3 = n (frames to render)
// Uses: NEON helpers _exp4_ps_asm, _sin4_ps_asm (available in active dir).
// Processes frames in groups of 4 using vector maths; any leftover <4 frames
// are currently ignored (will be handled in a later tail-pass patch).
// Structure offsets (see fm_voice.h):
//   0  sr (float32)
//   4  carrier_freq (float32)
//   8  ratio (float32)
//   12 index0 (float32)
//   16 amp (float32)
//   20 decay (float32)
//   24 len (uint32)
//   28 pos (uint32)
//   32 carrier_phase (float32)
//   36 mod_phase (float32)

_fm_voice_process:
    // Prologue ─────────────────────────────────────────────────────────────
    // Reserve a 192-byte fixed stack frame and save every callee-saved
    // GPR/NEON register we are going to clobber.  The stack pointer remains
    // constant for the entire function to avoid Heisenbugs caused by dynamic
    // pushes/pops inside hot loops.

    // Frame layout (offset → contents)
    //  0  : x19, x20
    // 16  : x21, x22
    // 32  : x23, x24
    // 192 : x27, x28 (immutable L/R bases)
    // 64  : q16, q17
    // 96  : q18, q19
    // 128 : q20, q21
    // 160 : q22, q23

    // Reserve a 2 KiB frame so positive offsets stay clear of the guard page
    sub     sp, sp, #2048
    stp     x19, x20, [sp, #0]
    stp     x21, x22, [sp, #16]
    stp     x23, x24, [sp, #32]
    stp     q16, q17, [sp, #64]
    stp     q18, q19, [sp, #96]
    stp     q20, q21, [sp, #128]
    stp     q22, q23, [sp, #160]
    stp     x27, x28, [sp, #192]

    // ---------------------------------------------------------------------
    // Set up immutable base pointers for L / R buffers in x19 / x20.
    mov     x19, x1              // L base (callee-saved, stable)
    mov     x20, x2              // R base (callee-saved, stable)
    // Debug sentinels – store immutable base pointers so LLDB can watch
    // these slots for unexpected writes that would indicate register
    // clobber inside the vector loop.
    str     x19, [sp, #256]     // sentinel_L
    str     x20, [sp, #264]     // sentinel_R
    // Copy frame count (n) into our working counter w12.
    mov     w12, w3
    cbz     w12, .Ldone          // nothing to do

    // Load pos and len (uint32)
    ldr     w4,  [x0, #28]       // pos
    ldr     w5,  [x0, #24]       // len
    cmp     w4, w5
    b.hs    .Ldone               // voice already finished

    // ---------------- Parameter load ------------------------------------
    ldr     s16, [x0]            // sr
    ldr     s26, [x0, #4]        // carrier_freq
    ldr     s27, [x0, #8]        // ratio
    ldr     s15, [x0, #12]       // index0
    ldr     s25, [x0, #16]       // amp
    ldr     s21, [x0, #20]       // decay
    ldr     s22, [x0, #32]       // carrier_phase (cp)
    ldr     s23, [x0, #36]       // mod_phase (mp)

    // ----------- Constants & increments ---------------------------------
    adrp    x10, Lfm_tau@PAGE
    add     x10, x10, Lfm_tau@PAGEOFF
    ldr     s24, [x10]           // TAU

    fmul    s12, s24, s26        // TAU * f_c (temp)
    fdiv    s24, s12, s16        // s24 := c_inc
    // m_inc = c_inc * ratio
    fmul    s12, s24, s27        // s12 := m_inc
    fmov    s14, s12             // mirror m_inc into s14 for NEON dup (v14)

    // Scalars used each iteration
    fmov    s13, #4.0            // constant 4.0
    fmul    s31, s24, s13        // 4 * c_inc (store in s31)
    fmul    s30, s12, s13        // 4 * m_inc (store in s30)

    // Pre-compute sr_inv and –decay
    fdiv    s29, s13, s13        // 1.0
    fdiv    s29, s29, s16        // 1 / sr
    fneg    s30, s21             // –decay

    // Lane vector {0,1,2,3}
    adrp    x9, Lfm_const@PAGE
    add     x9, x9, Lfm_const@PAGEOFF
    ldr     q1, [x9]

    // Cache helper function addresses to avoid PLT trampoline stack churn
    adrp    x17, _exp4_ps_asm@PAGE   // x17 -> exp4 helper
    add     x17, x17, _exp4_ps_asm@PAGEOFF
    adrp    x18, sin4_ps_asm_internal@PAGE   // x18 -> sin4 helper (local, no PLT)
    add     x18, x18, sin4_ps_asm_internal@PAGEOFF

// ====================== Vector Loop =====================================
.Lloop:
    // Guard against unexpected SP modification during loop
    mov     x24, sp              // snapshot SP at loop start
    // Reload immutable base pointers each iteration
    ldr     x19, [sp, #256]
    ldr     x20, [sp, #264]
    // Use immutable base pointers held in x19/x20 (callee-saved) for mixing
    mov     x27, x19
    mov     x28, x20
    // Exit conditions ----------------------------------------------------
    cmp     w12, #4
    blt     .Ltail
    sub     w7, w5, w4            // remaining in note
    cmp     w7, #4
    blt     .Ltail

    // cp_v = cp + lane*c_inc
    dup     v2.4s,  v22.s[0]
    dup     v3.4s,  v24.s[0]
    fmla    v2.4s,  v3.4s, v1.4s

    // mp_v = mp + lane*m_inc
    dup     v4.4s,  v23.s[0]
    fmov    s30, s14            // copy m_inc safely
    dup     v5.4s,  v30.s[0]
    fmla    v4.4s,  v5.4s, v1.4s

    // t_vec = (pos + lane) * sr_inv
    scvtf   s28,  w4          // convert pos to float
    dup     v6.4s,  v28.s[0]
    fadd    v6.4s,  v6.4s, v1.4s
    dup     v7.4s,  v29.s[0]
    fmul    v6.4s,  v6.4s, v7.4s

    // env_v = exp4_ps_asm( –decay * t_vec )
    dup     v8.4s,  v30.s[0]
    fmul    v0.4s,  v6.4s, v8.4s
    // Call helper (x19/x20 are callee-saved)
    mov     x23, x12             // backup loop counter
    mov     x24, sp              // snapshot SP before helper
    blr     x17
    cmp     sp, x24              // SP changed?
    b.eq    2f
    nop                     // disabled old debug trap 0xF01
2:
    // Restore immutable L/R bases in case helper clobbered x19/x20
    ldr     x19, [sp, #256]
    ldr     x20, [sp, #264]
    mov     x12, x23             // restore loop counter
    mov     v9.16b, v0.16b       // env_v

    // idx_v = env * index0
    dup     v10.4s, v15.s[0]
    fmul    v10.4s, v9.4s, v10.4s

    // inner = sin4_ps_asm(mp_v)
    mov     v0.16b, v4.16b
    mov     x23, x12
    mov     x24, sp
    blr     x18
    cmp     sp, x24
    b.eq    3f
    nop                     // disabled old debug trap 0xF02
3:
    ldr     x19, [sp, #256]
    ldr     x20, [sp, #264]
    mov     x12, x23
    mov     v11.16b, v0.16b

    // arg = cp_v + idx_v * inner
    fmul    v12.4s, v10.4s, v11.4s
    fadd    v12.4s, v2.4s, v12.4s

    // sample = sin(arg) * env * amp
    mov     v0.16b, v12.16b
    mov     x23, x12
    mov     x24, sp
    blr     x18
    cmp     sp, x24
    b.eq    4f
    nop                     // disabled old debug trap 0xF03
4:
    ldr     x19, [sp, #256]
    ldr     x20, [sp, #264]
    mov     x12, x23
    mov     v13.16b, v0.16b
    fmul    v13.4s, v13.4s, v9.4s
    fmov    s28, s25          // copy amp scalar safely
    dup     v29.4s, v28.s[0]
    fmul    v13.4s, v13.4s, v29.4s

    // Check SP integrity; if changed trigger breakpoint for debugging
    cmp     sp, x24
    b.eq    1f
    nop                     // disabled old debug trap 0xF00
1:
    // Ensure x27/x28 still hold base pointers
    mov     x27, x19
    mov     x28, x20

    // --------------------------------------------------------------------
    // DEBUG CHECK: ensure x27 (L base) still intact before we touch memory
    cmp     x27, x19
    b.eq    9f
    brk     #0xF06           // pointer clobbered – trap to LLDB
9:
    // Write mix into L / R buffers (additive)
    // x27/x28 hold immutable bases (callee-saved); no reload needed
    ubfx    x13, x4, #0, #32      // zero-extend pos
    lsl     x13, x13, #2          // *4 bytes per float
    add     x14, x27, x13         // L write pointer
    add     x15, x28, x13         // R write pointer

    ld1     {v27.4s}, [x14]
    fadd    v27.4s, v27.4s, v13.4s
    st1     {v27.4s}, [x14]

    ld1     {v26.4s}, [x15]
    fadd    v26.4s, v26.4s, v13.4s
    st1     {v26.4s}, [x15]

    // Advance phases / counters
    fadd    s22, s22, s31
    fadd    s23, s23, s30
    add     w4,  w4,  #4          // pos += 4

    subs    w12, w12, #4          // n -= 4
    b.ne    .Lloop                // continue if frames remain

// -------------------------------------------------------------------------
.Ltail:
    // (Optional scalar tail for <4 frames not yet implemented)

    // Store updated state back to struct
    str     s22, [x0, #32]
    str     s23, [x0, #36]
    str     w4,  [x0, #28]

// -------------------------------------------------------------------------
.Ldone:
    // Epilogue – restore saved regs and return
    ldp     q22, q23, [sp, #160]
    ldp     q20, q21, [sp, #128]
    ldp     q18, q19, [sp, #96]
    ldp     q16, q17, [sp, #64]
    // Restore saved x27/x28 (not used by this function but may have held caller state)
    ldp     x27, x28, [sp, #192]   // original caller values
    ldp     x23, x24, [sp, #32]
    ldp     x21, x22, [sp, #16]
    ldp     x19, x20, [sp, #0]
    add     sp, sp, #2048
    ret

// Constants
    .align 4
Lfm_const:
    .float 0.0, 1.0, 2.0, 3.0 
Lfm_tau:
    .float 6.2831855 