// AArch64 assembly implementation of noise_block
// void noise_block(rng_t *rng, float *out, uint32_t n)
//   rng: pointer to rng_t { uint64_t state; }
//   out: pointer to float buffer
//   n  : number of samples
// Generates white noise in range [-1,1)

	.text
	.align 2
	.globl _noise_block

// 64-bit constants for SplitMix64 algorithm
noise_consts64:
	.quad 0x9E3779B97F4A7C15   // GAMMA constant to add to state
	.quad 0xBF58476D1CE4E5B9   // MUL1 constant
	.quad 0x94D049BB133111EB   // MUL2 constant

// float constants
noise_consts32:
	.float 5.9604644775390625e-8    // 1/2^24
	.float 2.0
	.float 1.0

// Registers mapping (inside loop):
// x0 = rng*, x1 = out*, w2 = n, w3 = i (counter)
// x4 = tmp 64-bit value (state/z), x5 = const ptr
// s0 = float value, s1 = inv24, s2 = two, s3 = one

_noise_block:
	stp x29, x30, [sp, #-16]!
	mov x29, sp

	cmp w2, #0          // if n==0, return early
	b.eq 2f

	// Load pointers to constant tables
	adrp x5, noise_consts64@PAGE
	add  x5, x5, noise_consts64@PAGEOFF

	adrp x6, noise_consts32@PAGE
	add  x6, x6, noise_consts32@PAGEOFF

	ldr  s1, [x6]          // inv24 = 1/16777216
	ldr  s2, [x6, #4]      // 2.0
	ldr  s3, [x6, #8]      // 1.0

	mov w3, wzr            // i = 0
1:  // main loop
	// --- SplitMix64 ---
	ldr x4, [x0]           // load state
	ldr x7, [x5]           // GAMMA
	add x4, x4, x7         // state += GAMMA
	str x4, [x0]           // store updated state back

	// z = state
	// z ^= (z >> 30); z *= MUL1;
	mov x8, x4
	lsr x9, x8, #30
	eor x8, x8, x9
	ldr x9, [x5, #8]       // MUL1
	mul x8, x8, x9
	// z ^= (z >> 27); z *= MUL2;
	lsr x9, x8, #27
	eor x8, x8, x9
	ldr x9, [x5, #16]      // MUL2
	mul x8, x8, x9
	// z ^= (z >> 31);
	lsr x9, x8, #31
	eor x8, x8, x9

	// Take lower 32 bits and shift right by 8
	mov w10, w8            // w10 = low32(z)
	lsr w10, w10, #8       // 24-bit value

	// Convert to float in [0,1)
	ucvtf s0, w10          // s0 = float(u)
	fmul s0, s0, s1        // * (1/2^24)

	// scale to [-1,1): v = s0 * 2 - 1
	fmul s0, s0, s2
	fsub s0, s0, s3

	str s0, [x1], #4       // store sample and post-inc ptr

	// increment counter and loop
	add w3, w3, #1
	cmp w3, w2
	b.lo 1b

2:
	ldp x29, x30, [sp], #16
	ret 