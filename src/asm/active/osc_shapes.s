	.text
	.align 2
	.globl _osc_saw_block
	.globl _osc_square_block
	.globl _osc_triangle_block

// Shared constant
osc_TAU_const:
	.float 6.2831855
osc_half_TAU:
	.float 3.1415927
osc_two_const:
	.float 2.0
osc_one_const:
	.float 1.0
osc_neg_one_const:
	.float -1.0

// Helper macro to compute phase_inc in s2, load TAU in s3
.macro PREP_PHASE
	adrp x9, osc_TAU_const@PAGE
	add  x9, x9, osc_TAU_const@PAGEOFF
	ldr  s3, [x9]
	fdiv s4, s3, s1   // TAU / sr
	fmul s2, s4, s0   // phase_inc
	ldr s5, [x0]      // ph
	mov w3, wzr       // i=0
.endm

// void osc_saw_block(osc_t*, float*, n, freq, sr)
_osc_saw_block:
	stp x29, x30, [sp, #-16]!
	mov x29, sp
	mov x8, x0
	PREP_PHASE
	adrp x10, osc_two_const@PAGE
	add  x10, x10, osc_two_const@PAGEOFF
	ldr  s6, [x10]    // 2.0
0:
	cmp w3, w2
	b.hs 1f
	// frac = ph / TAU => s7
	fdiv s7, s5, s3
	// out = 2*frac -1
	fmul s7, s7, s6
	adrp x11, osc_one_const@PAGE
	add  x11, x11, osc_one_const@PAGEOFF
	ldr  s8, [x11]
	fsub s7, s7, s8
	str s7, [x1], #4
	// ph += inc; wrap
	fadd s5, s5, s2
	fcmpe s5, s3
	b.lt 2f
	fsub s5, s5, s3
2:
	add w3, w3, #1
	b 0b
1:
	str s5, [x8]
	ldp x29, x30, [sp], #16
	ret

// void osc_square_block(...)
_osc_square_block:
	stp x29, x30, [sp, #-16]!
	mov x29, sp
	mov x8, x0
	PREP_PHASE
	adrp x12, osc_half_TAU@PAGE
	add  x12, x12, osc_half_TAU@PAGEOFF
	ldr  s9, [x12]    // half TAU
	adrp x13, osc_one_const@PAGE
	add  x13, x13, osc_one_const@PAGEOFF
	ldr  s10, [x13]
	adrp x14, osc_neg_one_const@PAGE
	add  x14, x14, osc_neg_one_const@PAGEOFF
	ldr  s11, [x14]
Lsq_loop:
	cmp w3, w2
	b.hs Lsq_done
	// out = (ph < half_tau) ? 1 : -1
	fcmpe s5, s9
	fcsel s7, s10, s11, lt
	str s7, [x1], #4
	fadd s5, s5, s2
	fcmpe s5, s3
	b.lt Lsq_wrap
	fsub s5, s5, s3
Lsq_wrap:
	add w3, w3, #1
	b Lsq_loop
Lsq_done:
	str s5, [x8]
	ldp x29, x30, [sp], #16
	ret

// void osc_triangle_block(...)
_osc_triangle_block:
	stp x29, x30, [sp, #-16]!
	mov x29, sp
	mov x8, x0
	PREP_PHASE
	adrp x15, osc_two_const@PAGE
	add  x15, x15, osc_two_const@PAGEOFF
	ldr  s6, [x15]
	adrp x16, osc_one_const@PAGE
	add  x16, x16, osc_one_const@PAGEOFF
	ldr  s7, [x16]
Ltr_loop:
	cmp w3, w2
	b.hs Ltr_done
	// frac = ph/Tau -> s9
	fdiv s9, s5, s3
	// temp = 2*frac -1
	fmul s10, s9, s6
	fsub s10, s10, s7
	// abs
	fabs s10, s10
	// val = 2*abs(temp) -1
	fmul s10, s10, s6
	fsub s10, s10, s7
	str s10, [x1], #4
	fadd s5, s5, s2
	fcmpe s5, s3
	b.lt Ltr_wrap
	fsub s5, s5, s3
Ltr_wrap:
	add w3, w3, #1
	b Ltr_loop
Ltr_done:
	str s5, [x8]
	ldp x29, x30, [sp], #16
	ret 