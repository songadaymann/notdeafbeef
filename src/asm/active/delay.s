.text
.align 2
.globl _delay_process_block

// -----------------------------------------------------------------------------
// void delay_process_block(delay_t *d, float *L, float *R, uint32_t n, float feedback)
//    x0 = delay_t* { float *buf; uint32_t size; uint32_t idx; }
//    x1 = L buffer
//    x2 = R buffer
//    w3 = n samples
//    s0 = feedback amount
// Stereo ping-pong delay: L feeds R, R feeds L
// -----------------------------------------------------------------------------
_delay_process_block:
    // Prologue – use 512-byte frame; save x27/x28 at offset #480 (512-64) giving
    // the maximum distance LLDP allows (>=480 <=504) to avoid overlapping caller
    // memory even in fast execution.
    stp x29, x30, [sp, #-512]!
    stp q8,  q9,  [sp, #112]
    stp q10, q11, [sp, #144]
    stp q12, q13, [sp, #176]
    stp q14, q15, [sp, #208]
    mov x29, sp
    stp x19, x20, [sp, #16]
    stp x21, x22, [sp, #32]
    stp x23, x24, [sp, #48]
    stp x25, x26, [sp, #64]
    stp x27, x28, [sp, #480]

    // Load struct members (buf,size,idx) into convenient regs
    ldr x4, [x0]       // buf*
    ldr w5, [x0, #8]   // size
    ldr w6, [x0, #12]  // idx

    // Early-out if n==0
    cbz w3, Ldone

    // --- PRE-WRAP BUG FIX ----------------------------------------------------
    // Make absolutely sure idx is in range BEFORE first buffer access.
    cmp w6, w5          // idx >= size ?
    csel w6, wzr, w6, hs// if so wrap to 0
    // ------------------------------------------------------------------------

    mov w7, wzr        // loop counter i

Lloop:
    // Break conditions
    cmp w7, w3
    b.hs Lstore_idx     // i >= n → exit loop

    // Calculate &buf[idx*2]
    // (each sample is float = 4 bytes, stereo interleaved)
    // addr = buf + idx*8
    lsl w8, w6, #3      // w8 = idx*8
    add x9, x4, x8      // x9 = &buf[idx*2]

    // Load delayed samples
    ldp s1, s2, [x9]    // s1 = yl, s2 = yr

    // Load input samples (post-increment L/R ptrs)
    ldr s3, [x1]        // L[i]
    ldr s4, [x2]        // R[i]

    // buf[idx*2]   = L + yr*feedback
    fmadd s5, s2, s0, s3
    // buf[idx*2+1] = R + yl*feedback
    fmadd s6, s1, s0, s4
    stp  s5, s6, [x9]

    // Add delayed signal to dry samples
    fadd s3, s3, s1     // L[i] = dryL + yl
    fadd s4, s4, s2     // R[i] = dryR + yr
    str  s3, [x1], #4   // write & advance L*
    str  s4, [x2], #4   // write & advance R*

    // Increment and wrap idx
    add w6, w6, #1
    cmp w6, w5
    csel w6, wzr, w6, hs

    // Next sample
    add w7, w7, #1
    b    Lloop

Lstore_idx:
    // Store updated idx back to struct
    str w6, [x0, #12]

Ldone:
    // Epilogue – mirror prologue order
    ldp x27, x28, [sp, #480]
    ldp q14, q15, [sp, #208]
    ldp q12, q13, [sp, #176]
    ldp q10, q11, [sp, #144]
    ldp q8,  q9,  [sp, #112]
    ldp x29, x30, [sp]
    add sp, sp, #512
    ret