	.text
	.align  2
	.globl _euclid_pattern
_euclid_pattern:                     // void euclid_pattern(int pulses,int steps,uint8_t *out)
	stp     x29, x30, [sp, #-16]!      // prologue
	mov     x29, sp

	mov     w3, wzr                   // bucket = 0
	mov     w4, wzr                   // i = 0 (loop counter)

1:  cmp     w4, w1                  // while (i < steps)
	b.ge    2f

	add     w3, w3, w0               // bucket += pulses
	cmp     w3, w1
	b.lt    3f
	sub     w3, w3, w1               // bucket -= steps
	mov     w5, #1                   // out[i] = 1
	b       4f
3:
	mov     w5, #0                   // out[i] = 0
4:	add     x6, x2, x4              // &out[i]
	strb    w5, [x6]

	add     w4, w4, #1               // i++
	b       1b

2:	ldp     x29, x30, [sp], #16     // epilogue
	ret 