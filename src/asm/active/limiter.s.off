// AArch64 assembly implementation of soft-knee limiter
// void limiter_process(limiter_t *l, float *L, float *R, uint32_t n)
//   l: x0 = pointer to limiter_t { attack_coeff, release_coeff, envelope, threshold, knee_width }
//   L: x1 = left channel buffer
//   R: x2 = right channel buffer
//   n: w3 = number of samples
//
// Uses soft-knee compression with envelope follower

	.text
	.align 2
	.globl _limiter_process

limiter_consts:
	.float 20.0
	.float 2.0
	.float 10.0
	.float -0.5
	.float 1.0

_limiter_process:
	// Save registers we'll use
	stp x29, x30, [sp, #-96]!
	stp x19, x20, [sp, #16]
	stp x21, x22, [sp, #32]
	mov x29, sp
	
	// Early exit if n == 0
	cbz w3, done
	
	// Save arguments
	mov x19, x0              // limiter struct
	mov x20, x1              // L pointer
	mov x21, x2              // R pointer
	mov w22, w3              // n
	
	// Load limiter parameters
	ldr s0, [x19]            // attack_coeff
	ldr s1, [x19, #4]        // release_coeff
	ldr s2, [x19, #8]        // envelope
	ldr s3, [x19, #12]       // threshold
	ldr s4, [x19, #16]       // knee_width
	
	// Store parameters on stack for reloading after function calls
	str s0, [sp, #48]        // attack_coeff
	str s1, [sp, #52]        // release_coeff
	str s3, [sp, #56]        // threshold
	str s4, [sp, #60]        // knee_width
	str s2, [sp, #64]        // envelope (will be updated)
	
	// Calculate knee bounds and store
	fmov s5, #-0.5
	fmul s5, s4, s5          // -knee_width/2
	str s5, [sp, #68]
	fneg s6, s5              // knee_width/2
	str s6, [sp, #72]
	
	// Main loop
	mov w23, wzr             // i = 0
	
loop:
	// Load samples
	ldr s7, [x20, w23, sxtw #2]   // L[i]
	ldr s8, [x21, w23, sxtw #2]   // R[i]
	
	// Get absolute values and peak
	fabs s9, s7              // |L[i]|
	fabs s10, s8             // |R[i]|
	fmax s11, s9, s10        // peak = max(|L|, |R|)
	
	// Load current envelope
	ldr s2, [sp, #64]
	
	// Envelope follower
	fcmp s11, s2
	b.le 1f
	
	// Attack: env = peak + att * (env - peak)
	ldr s0, [sp, #48]        // attack_coeff
	fsub s12, s2, s11
	fmadd s2, s0, s12, s11
	b 2f
	
1:	// Release: env = peak + rel * (env - peak)
	ldr s1, [sp, #52]        // release_coeff
	fsub s12, s2, s11
	fmadd s2, s1, s12, s11
	
2:	// Store updated envelope
	str s2, [sp, #64]
	
	// Calculate overshoot_db = 20 * log10(env / thresh)
	ldr s3, [sp, #56]        // threshold
	fdiv s0, s2, s3          // env / thresh
	
	// Save sample values before function call
	str s7, [sp, #76]        // L[i]
	str s8, [sp, #80]        // R[i]
	
	// Call log10f
	bl _log10f
	fmov s12, #20.0
	fmul s13, s0, s12        // overshoot_db = 20 * log10(...)
	
	// Reload samples
	ldr s7, [sp, #76]
	ldr s8, [sp, #80]
	
	// Calculate gain_reduction_db
	fmov s14, wzr            // gain_reduction_db = 0
	
	ldr s5, [sp, #68]        // -knee_width/2
	fcmp s13, s5
	b.le 3f                  // no reduction if below knee
	
	ldr s6, [sp, #72]        // knee_width/2
	fcmp s13, s6
	b.ge 4f                  // hard limit if above knee
	
	// Soft knee calculation
	fsub s15, s13, s5        // overshoot_db + knee_width/2
	fmul s15, s15, s15       // square it
	ldr s4, [sp, #60]        // knee_width
	fmov s16, #2.0
	fmul s16, s16, s4        // 2 * knee_width
	fdiv s14, s15, s16       // gain_reduction_db
	b 3f
	
4:	// Hard limiting
	fmov s14, s13
	
3:	// Convert to linear gain
	fneg s15, s14            // -gain_reduction_db
	fmov s16, #20.0
	fdiv s1, s15, s16        // -gain_reduction_db / 20
	
	// Save before powf
	str s7, [sp, #76]
	str s8, [sp, #80]
	
	// Call powf(10, exponent)
	fmov s0, #10.0
	bl _powf
	fmov s17, s0             // gain
	
	// Reload samples
	ldr s7, [sp, #76]
	ldr s8, [sp, #80]
	
	// Apply gain if < 1.0
	fmov s18, #1.0
	fcmp s17, s18
	b.ge 5f
	
	fmul s7, s7, s17
	fmul s8, s8, s17
	
5:	// Store processed samples
	str s7, [x20, w23, sxtw #2]
	str s8, [x21, w23, sxtw #2]
	
	// Loop control
	add w23, w23, #1
	cmp w23, w22
	b.lo loop
	
	// Store final envelope
	ldr s2, [sp, #64]
	str s2, [x19, #8]
	
done:
	// Restore registers
	ldp x19, x20, [sp, #16]
	ldp x21, x22, [sp, #32]
	ldp x29, x30, [sp], #96
	ret 