	.text
	.align 2
	.globl _osc_sine_block
// void osc_sine_block(osc_t *o, float *out, uint32_t n, float freq, float sr)
// x0=o, x1=out, w2=n, s0=freq, s1=sr
// Strategy: compute phase_inc = TAU*freq/sr once, then loop scalar using sinf

osc_sine_consts:
	.float 6.2831855            // TAU

_osc_sine_block:
	// Allocate stack: x29/x30 + phase + phase_inc + TAU (16 + 12 = 28, round to 32)
	stp x29, x30, [sp, #-32]!
	mov x29, sp
	
	// Save arguments we need to preserve
	mov x8, x0          // save osc pointer
	mov x9, x1          // save out pointer
	mov w10, w2         // save n
	
	// load TAU constant
	adrp x11, osc_sine_consts@PAGE
	add  x11, x11, osc_sine_consts@PAGEOFF
	ldr s3, [x11]       // TAU
	
	// phase_inc = TAU * freq / sr
	fdiv s4, s3, s1     // TAU / sr
	fmul s2, s4, s0     // phase_inc
	
	// Store constants on stack
	str s2, [sp, #16]   // phase_inc at sp+16
	str s3, [sp, #20]   // TAU at sp+20
	
	// Load initial phase
	ldr s0, [x8]        // current phase
	str s0, [sp, #24]   // phase at sp+24
	
	mov w11, wzr        // i = 0
	cmp w10, #0
	b.eq 1f             // if n==0 skip loop

0:
	// Load phase for sinf
	ldr s0, [sp, #24]
	bl _sinf
	str s0, [x9], #4    // store result and advance pointer
	
	// phase += phase_inc
	ldr s0, [sp, #24]   // reload phase
	ldr s1, [sp, #16]   // reload phase_inc
	fadd s0, s0, s1
	
	// wrap if phase >= TAU
	ldr s2, [sp, #20]   // reload TAU
	fcmpe s0, s2
	b.lt 2f
	fsub s0, s0, s2
2:
	str s0, [sp, #24]   // store updated phase
	
	add w11, w11, #1
	cmp w11, w10
	b.lo 0b

1:
	// store phase back to osc structure
	ldr s0, [sp, #24]
	str s0, [x8]
	
	ldp x29, x30, [sp], #32
	ret 