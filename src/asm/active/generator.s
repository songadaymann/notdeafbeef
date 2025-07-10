	.text
	.align 2
	.globl _generator_mix_buffers_asm

// Assembly stubs – we override only generator_process; keep C generator_init
	.globl _generator_process
	// (no _generator_init symbol here; C version remains)

_generator_process:
	// Args: x0 = g, x1 = L, x2 = R, w3 = num_frames

	// Prologue – save frame pointer & callee-saved regs (x19-x22)
	stp x29, x30, [sp, #-128]!   // reserve 128-byte fixed frame (was 96)
	mov x29, sp
	stp x19, x20, [sp, #16]
	stp x21, x22, [sp, #32]
	stp x23, x24, [sp, #48]
	stp x25, x26, [sp, #64]
	stp x27, x28, [sp, #80]

	// Early exit if no frames
	cbz w3, .Lgp_epilogue

	// Preserve output buffer pointers before we repurpose argument regs
	mov x19, x1            // L
	mov x20, x2            // R
	mov x24, x0            // g pointer
	// (dup line removed)
	// (generator pointer already preserved earlier)

	// Preserve num_frames in callee-saved x21 for later reuse
	mov x21, x3            // x21 = num_frames (32-bit valid)

	// ---------------------------------------------------------------------
	// Allocate contiguous scratch block on heap instead of stack
	// scratch_size = (num_frames * 16 + 15) & ~15      // 16-byte alignment
	lsl x22, x21, #4       // x22 = num_frames * 16  (bytes for 4 buffers)
	add x22, x22, #15
	bic x22, x22, #15      // align to 16 bytes

	// malloc(scratch_size)
	mov x0, x22
	bl _malloc
	mov x25, x0            // x25 = Ld base (scratch start)

	// bytes_per_buffer = num_frames * 4
	lsl x5, x21, #2        // x5 = bytes per buffer

	// Compute remaining scratch pointers
	add x26, x25, x5       // Rd = Ld + size
	add x27, x26, x5       // Ls
	add x28, x27, x5       // Rs

	// Prepare arguments for generator_clear_buffers_asm
	mov x0, x25            // Ld
	add x1, x0, x5         // Rd = Ld + bytes_per_buffer
	add x2, x1, x5         // Ls
	add x3, x2, x5         // Rs
	mov w4, w21            // num_frames

	stp x21, x22, [sp, #96]     // save frames_rem & x22 inside fixed frame
	bl _generator_clear_buffers_asm
	ldp x21, x22, [sp, #96]     // restore w21, x22 (sp unchanged)

	// ---------------------------------------------------------------------
	// Skip explicit memset on L/R since they will be fully overwritten by
	// subsequent processing/mixing.

	// ---------------------------------------------------------------------
	// Slice-2: Outer frame loop over musical steps (state advance only)

	// Register assignments:
	//   x24 = g (generator*) – set now
	// Use x10 as pointer to timing/event fields (base = g + 4352)
	add x10, x24, #0x1000    // x10 = g + 4096
	add x10, x10, #0x128     // +296 => g + 4392 (event_idx)

	ldr w9, [x24, #12]      // w9 = step_samples (offset 12 bytes)
	ldr w8, [x10, #8]       // w8 = pos_in_step (event base + 8)

	// TOTAL_STEPS constant
	mov w13, #32           // for wrap-around comparison

	// After generator_clear_buffers_asm call and before outer loop label
	// Save scratch base pointers for later voice processing
	// x25..x28 already set to scratch pointers
	
	mov w23, wzr            // frames_done = 0 (will live in w23/x23)

.Lgp_loop:
	ldr w9, [x24, #12]           // reload step_samples each iteration
	cbz w21, .Lgp_after_loop      // frames_rem == 0 ? done

	// ----- DEBUG: dump counters at loop start -----
.if 0
	stp x0, x1, [sp, #-16]!   // save caller-saved regs we'll clobber
	stp x2, x3, [sp, #-16]!
	adrp x0, .Ldbg_pre_fmt@PAGE
	add  x0, x0, .Ldbg_pre_fmt@PAGEOFF
	mov  w1, w21   // frames_rem
	mov  w2, w9    // step_samples
	mov  w3, w8    // pos_in_step
	bl   _printf
	ldp x2, x3, [sp], #16      // restore
	ldp x0, x1, [sp], #16      // restore
.endif
	// ---------------------------------------------

	// Slice-3: Trigger events at step start
	cbnz w8, .Lgp_trigger_skip                   // Only trigger when pos_in_step == 0
	// Preserve caller-saved x8/x9 that hold pos_in_step & scratch before calling C helper
	stp x8, x9, [sp, #112]        // save into fixed 128-byte frame (offsets 112-127)
	mov x0, x24                   // x0 = g pointer
	bl _generator_trigger_step
	ldp x8, x9, [sp, #112]        // restore registers (keeps sp constant)

.Lgp_trigger_skip:
	// Recompute event/state base pointer after external calls may clobber x10
	add x10, x24, #0x1000
	add x10, x10, #0x128    // x10 = &g->event_idx
	ldr w9, [x24, #12]

	// Reload constant step_samples in case caller-saved w9 was clobbered
	ldr w9, [x24, #12]           // w9 = step_samples (offset 12 bytes)
	// frames_to_step_boundary = step_samples - pos_in_step
	sub w10, w9, w8              // w10 = frames_to_step_boundary  (no slice-shortening)
	// FM sustain fix: if pos_in_step == 0 and frames_to_step_boundary > 1, decrement by 1 so
	// that voices (particularly fm_voice) spread notes over at least two slices. This mirrors
	// the logic added in generator.c (Round 23 fix).
	cbnz w8, 1f                 // if pos_in_step != 0, skip
	cmp  w10, #1
	ble 1f                      // if boundary <=1, nothing to shorten
	sub  w10, w10, #1           // frames_to_step_boundary -= 1
1:
	// frames_to_process = min(frames_rem, frames_to_step_boundary)
	cmp w21, w10
	b.lt 1f
	mov w11, w10
	b 2f
1:	mov w11, w21
2:
	// ---------- Slice-4: Voice processing + mixing ----------
	// Save frames_to_process into callee-saved x22 to survive C call (zero-extend to avoid garbage high bits)
	mov w22, w11               // preserve w11, zeroing upper 32 bits of x22

	// Compute byte offset into scratch/output for frames_done
	lsl x12, x23, #2            // x12 = frames_done * 4 (bytes)

	// Scratch pointers for this sub-block
	add x13, x25, x12           // Ld ptr
	add x14, x26, x12           // Rd ptr
	add x15, x27, x12           // Ls ptr
	add x16, x28, x12           // Rs ptr
	// Output pointers
	add x17, x19, x12           // L dest
	add x18, x20, x12           // R dest

	// Call voice processor (preserve x21 across call)
	stp x21, x22, [sp, #96]     // save frames_rem & x22 inside fixed frame

	mov x0, x24                 // g pointer
	mov x1, x13                 // Ld
	mov x2, x14                 // Rd
	mov x3, x15                 // Ls
	mov x4, x16                 // Rs
	mov w5, w11                 // num_frames
	bl _generator_process_voices

	ldp x21, x22, [sp, #96]     // restore w21, x22 (sp unchanged)

	// Recompute event/state base pointer after _generator_process_voices (x10 may be clobbered)
	add x10, x24, #0x1000
	add x10, x10, #0x128    // x10 = &g->event_idx

	// Restore w11 from x22 after helper
	mov w11, w22               // restore frames_to_process
	// Reload pos_in_step since w8 is caller-clobbered
	ldr w8, [x10, #8]

    // ----- TRACE1: after voice processing -----
.if 0
    stp x0, x1, [sp, #-16]!   // save regs clobbered by printf
    adrp x0, .Ldbg_post1_fmt@PAGE
    add  x0, x0, .Ldbg_post1_fmt@PAGEOFF
    mov  w1, w21   // frames_rem (remaining)
    mov  w2, w11   // frames_to_process for this slice
    mov  w3, w8    // current pos_in_step
    bl   _printf
    ldp x0, x1, [sp], #16      // restore regs
.endif

	// Recompute scratch & output pointers after C helper (all caller-saved regs may be clobbered)
	lsl x12, x23, #2            // byte offset = frames_done * 4
	add x13, x25, x12           // Ld ptr
	add x14, x26, x12           // Rd ptr
	add x15, x27, x12           // Ls ptr
	add x16, x28, x12           // Rs ptr
	add x17, x19, x12           // L dest
	add x18, x20, x12           // R dest

	// Mix drums + synths into output buffers
	mov x0, x17                 // L out
	mov x1, x18                 // R out
	mov x2, x13                 // Ld
	mov x3, x14                 // Rd
	mov x4, x15                 // Ls
	mov x5, x16                 // Rs
	mov w6, w11                 // num_frames
	bl _generator_mix_buffers_asm

	// ----- SCRATCH RMS PROBE (debug – first slice only) -----
	.if 0
		cbnz w23, .Lskip_scratch_rms    // only when frames_done == 0

		// Save caller-saved regs we will clobber (x0-x3)
		stp x0, x1, [sp, #-16]!
		stp x2, x3, [sp, #-16]!

		// ---- drums scratch RMS (Ld/Rd) ----
		mov x0, x13      // Ld
		mov x1, x14      // Rd
		mov w2, w11      // num_frames in slice
		bl  _generator_compute_rms_asm   // s0 = RMS
		fmov w4, s0                      // raw bits -> w4

		// ---- synth scratch RMS (Ls/Rs) ----
		mov x0, x15      // Ls
		mov x1, x16      // Rs
		mov w2, w11
		bl  _generator_compute_rms_asm   // s0 = RMS
		fmov w5, s0                      // raw bits -> w5

		// printf("SCR drums=%u synth=%u n=%u\n", drums_bits, synth_bits, frames_to_process)
		adrp x0, .Ldbg_scratch_fmt@PAGE
		add  x0, x0, .Ldbg_scratch_fmt@PAGEOFF
		mov  w1, w4
		mov  w2, w5
		mov  w3, w11
		bl   _printf

		// Restore clobbered regs
		ldp x2, x3, [sp], #16
		ldp x0, x1, [sp], #16

	.Lskip_scratch_rms:
	.endif

	// ----- RMS DEBUG -----
	.if 0
	    // Save x0–x3 into unused area of 128-byte fixed frame (keeps sp constant)
	    stp x0, x1, [x29, #96]
	    stp x2, x3, [x29, #112]

	    mov x0, x17                       // L buffer pointer
	    mov x1, x18                       // R buffer pointer
	    mov w2, w11                       // num_frames this slice
	    bl _generator_compute_rms_asm     // s0 = RMS (float)

	    // Print raw IEEE bits so we avoid float formatting overhead
	    fmov w1, s0                       // RMS bits → w1
	    adrp x0, .Ldbg_rms_fmt@PAGE       // format string "%u\n"
	    add  x0, x0, .Ldbg_rms_fmt@PAGEOFF
	    bl  _printf

	    // Restore x0–x3
	    ldp x2, x3, [x29, #112]
	    ldp x0, x1, [x29, #96]
	.endif
	// ----- END RMS DEBUG -----

	// DEBUG PRINT BEGIN
.if 0
	stp x21, x22, [sp, #-16]!        // save frames_rem and spare callee-saved slot
	stp x8, x11, [sp, #-16]!         // save live regs for printf args
	adrp x0, .Ldbg_fmt@PAGE
	add  x0, x0, .Ldbg_fmt@PAGEOFF
	mov  w1, w21  // frames_rem
	mov  w2, w11  // frames_to_process
	mov  w3, w8   // pos_in_step
	bl   _printf
	ldp x8, x11, [sp], #16           // restore
	ldp x21, x22, [sp], #16          // restore frames_rem
.endif
	// DEBUG PRINT END

	// Advance counters
	add w8, w8, w11              // pos_in_step += frames_to_process
    // write back updated pos_in_step to struct
    add x10, x24, #0x1000
    add x10, x10, #0x128   // correct base for event/state block (g + 4392)
    str w8, [x10, #8]
	sub w21, w21, w11            // frames_rem  -= frames_to_process
	add w23, w23, w11            // frames_done += frames_to_process

	// Check if step boundary reached
	cmp w8, w9
	b.lt .Lgp_loop

	// Boundary reached – reset pos_in_step and advance step
	mov w8, wzr
	// Recompute event/state base pointer again (x10 may be clobbered by helpers)
	add x10, x24, #0x1000
	add x10, x10, #0x128   // x10 = &g->event_idx
	str w8, [x10, #8]       // write back pos_in_step = 0 to generator struct
	ldr w12, [x10, #4]        // w12 = step (event base + 4)
	add w12, w12, #1
	cmp w12, w13
	b.lt 3f
	mov w12, wzr
	str wzr, [x10]            // event_idx reset
3:	str w12, [x10, #4]
	b .Lgp_loop

.Lgp_after_loop:
	// Store updated pos_in_step back
	str w8, [x10, #8]

	// Deallocate scratch (free)
	mov x0, x25
	bl _free

	// TEMP BYPASS delay & limiter for debug RMS (define DEBUG_SKIP_POST_DSP)
	#ifdef DEBUG_SKIP_POST_DSP
	    b .Lgp_epilogue
	#endif

	// ---------------------------------------------------------------------
	// Slice-5: Apply Delay & Limiter (C implementations)
	// --------------------------------------------------
	// delay_process_block(&g->delay, L, R, num_frames, 0.45f);
	// limiter_process(&g->limiter, L, R, num_frames);

	// Prepare arguments for delay_process_block
	// x24 = g (preserved), x19 = L buffer, x20 = R buffer, w23 = total num_frames

	// x0 = &g->delay  (offset 4408 bytes)
	add x0, x24, #4096        // base offset
	add x0, x0, #312          // 4096 + 312 = 4408
	mov x1, x19               // L
	mov x2, x20               // R
	mov w3, w23               // n = num_frames
	// s0 = 0.45f  (IEEE-754 0x3EE66666)
	mov w4, #0x6666
	movk w4, #0x3EE6, lsl #16
	fmov s0, w4
	bl _delay_process_block

    #ifndef SKIP_LIMITER
    // Prepare arguments for limiter_process
    // x0 = &g->limiter (offset 4424 bytes)
    add x0, x24, #4096        // base offset
    add x0, x0, #328          // 4096 + 328 = 4424
    mov x1, x19               // L
    mov x2, x20               // R
    mov w3, w23               // n = num_frames
    bl _limiter_process
    #endif

	// Existing epilogue label below handles register restore and return

.Lgp_epilogue:
	// Early-exit path: deallocate scratch skipped (not allocated)
	ldp x27, x28, [x29, #80]
	ldp x25, x26, [x29, #64]
	ldp x23, x24, [x29, #48]
	ldp x21, x22, [x29, #32]
	ldp x19, x20, [x29, #16]
	ldp x29, x30, [sp], #128    // pop full 128-byte frame
	ret

/*
 * generator_mix_buffers_asm - NEON vectorized buffer mixing
 * -------------------------------------------------------
 * void generator_mix_buffers_asm(float *L, float *R, 
 *                                 const float *Ld, const float *Rd,
 *                                 const float *Ls, const float *Rs,
 *                                 uint32_t num_frames);
 *
 * Performs: L[i] = Ld[i] + Ls[i]  (drums + synths)
 *          R[i] = Rd[i] + Rs[i]  (drums + synths)
 *
 * Uses NEON to process 4 samples per iteration for maximum throughput.
 * This is the hot path that runs every audio frame in real-time.
 */

_generator_mix_buffers_asm:
	// Arguments: x0=L, x1=R, x2=Ld, x3=Rd, x4=Ls, x5=Rs, w6=num_frames
	
	// Early exit if no frames to process
	cbz w6, .Lmix_done
	
	// Calculate how many complete NEON vectors (4 samples) we can process
	lsr w7, w6, #2          // w7 = num_frames / 4 (complete vectors)
	and w8, w6, #3          // w8 = num_frames % 4 (remainder samples)
	
	// Process complete 4-sample vectors with NEON
	cbz w7, .Lmix_scalar    // Skip if no complete vectors
	
.Lmix_vector_loop:
	// Load 4 samples from each source buffer
	ld1 {v0.4s}, [x2], #16  // v0 = Ld[i..i+3], advance pointer
	ld1 {v1.4s}, [x3], #16  // v1 = Rd[i..i+3], advance pointer  
	ld1 {v2.4s}, [x4], #16  // v2 = Ls[i..i+3], advance pointer
	ld1 {v3.4s}, [x5], #16  // v3 = Rs[i..i+3], advance pointer
	
	// Vector addition: drums + synths
	fadd v4.4s, v0.4s, v2.4s  // v4 = Ld + Ls
	fadd v5.4s, v1.4s, v3.4s  // v5 = Rd + Rs
	
	// Store results to output buffers
	st1 {v4.4s}, [x0], #16   // L[i..i+3] = v4, advance pointer
	st1 {v5.4s}, [x1], #16   // R[i..i+3] = v5, advance pointer
	
	// Loop control
	subs w7, w7, #1
	b.ne .Lmix_vector_loop
	
.Lmix_scalar:
	// Handle remaining samples (0-3) with scalar operations
	cbz w8, .Lmix_done
	
.Lmix_scalar_loop:
	// Load single samples
	ldr s0, [x2], #4        // s0 = Ld[i]
	ldr s1, [x3], #4        // s1 = Rd[i]
	ldr s2, [x4], #4        // s2 = Ls[i]
	ldr s3, [x5], #4        // s3 = Rs[i]
	
	// Scalar addition
	fadd s4, s0, s2         // s4 = Ld[i] + Ls[i]
	fadd s5, s1, s3         // s5 = Rd[i] + Rs[i]
	
	// Store results
	str s4, [x0], #4        // L[i] = s4
	str s5, [x1], #4        // R[i] = s5
	
	// Loop control
	subs w8, w8, #1
	b.ne .Lmix_scalar_loop
	
.Lmix_done:
	ret

/*
 * generator_compute_rms_asm - NEON vectorized RMS calculation
 * ---------------------------------------------------------
 * float generator_compute_rms_asm(const float *L, const float *R, uint32_t num_frames);
 *
 * Computes RMS = sqrt(sum(L[i]² + R[i]²) / (num_frames * 2))
 * 
 * Uses NEON to process 4 samples per iteration:
 * - Load L[i..i+3] and R[i..i+3] 
 * - Square each (fmul)
 * - Add L² + R² (fadd)
 * - Accumulate in vector sum
 * - Final horizontal sum + sqrt in scalar
 */

	.globl _generator_compute_rms_asm

_generator_compute_rms_asm:
	// Arguments: x0=L, x1=R, w2=num_frames
	// Returns: s0 = RMS value
	
	// Early exit if no frames
	cbz w2, .Lrms_zero
	
	// Initialize accumulator vector to zero
	movi v16.4s, #0              // v16 = accumulator for vector sum
	fmov s17, wzr                // s17 = accumulator for scalar sum
	
	// Calculate how many complete NEON vectors (4 samples) we can process
	lsr w3, w2, #2               // w3 = num_frames / 4 (complete vectors)
	and w4, w2, #3               // w4 = num_frames % 4 (remainder samples)
	
	// Process complete 4-sample vectors with NEON
	cbz w3, .Lrms_scalar         // Skip if no complete vectors
	
.Lrms_vector_loop:
	// Load 4 samples from each buffer
	ld1 {v0.4s}, [x0], #16       // v0 = L[i..i+3], advance pointer
	ld1 {v1.4s}, [x1], #16       // v1 = R[i..i+3], advance pointer
	
	// Square the samples: L² and R²
	fmul v2.4s, v0.4s, v0.4s     // v2 = L[i]² for 4 samples
	fmul v3.4s, v1.4s, v1.4s     // v3 = R[i]² for 4 samples
	
	// Add L² + R² 
	fadd v4.4s, v2.4s, v3.4s     // v4 = L[i]² + R[i]² for 4 samples
	
	// Accumulate in sum vector
	fadd v16.4s, v16.4s, v4.4s   // accumulate
	
	// Loop control
	subs w3, w3, #1
	b.ne .Lrms_vector_loop
	
.Lrms_scalar:
	// Handle remaining samples (0-3) with scalar operations
	cbz w4, .Lrms_finalize
	
.Lrms_scalar_loop:
	// Load single samples
	ldr s0, [x0], #4             // s0 = L[i]
	ldr s1, [x1], #4             // s1 = R[i]
	
	// Square and add: L² + R²
	fmul s2, s0, s0              // s2 = L[i]²
	fmul s3, s1, s1              // s3 = R[i]²
	fadd s4, s2, s3              // s4 = L[i]² + R[i]²
	
	// Add to scalar accumulator  
	fadd s17, s17, s4            // accumulate scalar remainder
	
	// Loop control
	subs w4, w4, #1
	b.ne .Lrms_scalar_loop
	
.Lrms_finalize:
	// Horizontal sum of accumulator vector v16 → s0
	faddp v18.4s, v16.4s, v16.4s // pairwise add: [a+b, c+d, a+b, c+d]
	faddp s0, v18.2s             // final vector sum: (a+b) + (c+d)
	
	// Add scalar accumulator to vector sum
	fadd s0, s0, s17             // total_sum = vector_sum + scalar_sum
	
	// Convert num_frames to float and multiply by 2
	ucvtf s1, w2                 // s1 = (float)num_frames  
	fmov s2, #2.0                // s2 = 2.0
	fmul s1, s1, s2              // s1 = num_frames * 2
	
	// Divide sum by (num_frames * 2) to get mean
	fdiv s0, s0, s1              // s0 = mean = sum / (num_frames * 2)
	
	// Take square root to get RMS
	fsqrt s0, s0                 // s0 = sqrt(mean) = RMS
	
	ret
	
.Lrms_zero:
	// Return 0.0 if no frames
	fmov s0, wzr
	ret 

/*
 * generator_clear_buffers_asm - NEON vectorized buffer clearing
 * -----------------------------------------------------------
 * void generator_clear_buffers_asm(float *Ld, float *Rd, float *Ls, float *Rs, uint32_t num_frames);
 *
 * Clears (zeros) all 4 float buffers using NEON vector stores.
 * Replaces 4 memset() calls with optimized NEON operations.
 * 
 * Uses NEON to process 4 samples per iteration for maximum throughput.
 */

	.globl _generator_clear_buffers_asm

_generator_clear_buffers_asm:
	// Arguments: x0=Ld, x1=Rd, x2=Ls, x3=Rs, w4=num_frames
	
	// Early exit if no frames to process
	cbz w4, .Lclear_done
	
	// Initialize zero vector for NEON stores
	movi v0.4s, #0               // v0 = [0.0, 0.0, 0.0, 0.0]
	
	// Calculate how many complete NEON vectors (4 samples) we can process
	lsr w5, w4, #2               // w5 = num_frames / 4 (complete vectors)
	and w6, w4, #3               // w6 = num_frames % 4 (remainder samples)
	
	// Process complete 4-sample vectors with NEON
	cbz w5, .Lclear_scalar       // Skip if no complete vectors
	
.Lclear_vector_loop:
	// Store 4 zero samples to each buffer
	st1 {v0.4s}, [x0], #16       // Ld[i..i+3] = 0.0, advance pointer
	st1 {v0.4s}, [x1], #16       // Rd[i..i+3] = 0.0, advance pointer  
	st1 {v0.4s}, [x2], #16       // Ls[i..i+3] = 0.0, advance pointer
	st1 {v0.4s}, [x3], #16       // Rs[i..i+3] = 0.0, advance pointer
	
	// Loop control
	subs w5, w5, #1
	b.ne .Lclear_vector_loop
	
.Lclear_scalar:
	// Handle remaining samples (0-3) with scalar operations
	cbz w6, .Lclear_done
	
	// Zero value for scalar stores
	fmov s1, wzr                 // s1 = 0.0
	
.Lclear_scalar_loop:
	// Store single zero sample to each buffer
	str s1, [x0], #4             // Ld[i] = 0.0
	str s1, [x1], #4             // Rd[i] = 0.0
	str s1, [x2], #4             // Ls[i] = 0.0  
	str s1, [x3], #4             // Rs[i] = 0.0
	
	// Loop control
	subs w6, w6, #1
	b.ne .Lclear_scalar_loop
	
.Lclear_done:
	ret

/*
 * generator_rotate_pattern_asm - NEON vectorized pattern rotation
 * -------------------------------------------------------------
 * void generator_rotate_pattern_asm(uint8_t *pattern, uint8_t *tmp, uint32_t size, uint32_t rot);
 *
 * Rotates a uint8_t array by 'rot' positions: pattern[i] = old_pattern[(i+rot) % size]
 * Optimized for size=16 (STEPS_PER_BAR) using NEON EXT instruction.
 * 
 * For size=16: Single NEON register holds entire pattern, EXT performs rotation in one operation.
 */

	.globl _generator_rotate_pattern_asm

_generator_rotate_pattern_asm:
	// Arguments: x0=pattern, x1=tmp, w2=size, w3=rot
	
	// Early exit if no rotation needed
	cbz w3, .Lrot_done
	
	// Optimize for STEPS_PER_BAR = 16 case
	cmp w2, #16
	b.eq .Lrot_neon16
	
	// Fallback for other sizes: scalar implementation
	b .Lrot_scalar
	
.Lrot_neon16:
	// NEON optimization for 16-byte patterns (STEPS_PER_BAR)
	// Load entire 16-byte pattern into single NEON register
	ld1 {v0.16b}, [x0]
	
	// Build rotation index table: [rot, rot+1, rot+2, ..., rot+15] % 16
	and w3, w3, #15          // Ensure rot is 0-15 (rot % 16)
	
	// Build index table on stack
	sub sp, sp, #16          // Allocate 16 bytes on stack
	mov w4, wzr              // w4 = loop counter
	
.Lrot_build_indices:
	add w5, w4, w3           // w5 = i + rot
	and w5, w5, #15          // w5 = (i + rot) % 16
	strb w5, [sp, w4, uxtw]  // Store index to stack
	add w4, w4, #1           // i++
	cmp w4, #16
	b.lt .Lrot_build_indices
	
	// Load index table and use TBL for rotation
	ld1 {v2.16b}, [sp]       // Load index table
	tbl v1.16b, {v0.16b}, v2.16b  // Perform table lookup rotation
	
	// Store rotated pattern back and clean up stack
	st1 {v1.16b}, [x0]
	add sp, sp, #16          // Restore stack
	
	b .Lrot_done
	
.Lrot_scalar:
	// Scalar fallback for arbitrary sizes
	// Copy pattern to tmp buffer
	mov w4, wzr              // w4 = loop counter for memcpy
	
.Lrot_copy_loop:
	cmp w4, w2
	b.ge .Lrot_rotate_start
	ldrb w5, [x0, w4, uxtw]  // Load pattern[i]
	strb w5, [x1, w4, uxtw]  // Store to tmp[i]
	add w4, w4, #1
	b .Lrot_copy_loop
	
.Lrot_rotate_start:
	// Rotate: pattern[i] = tmp[(i+rot) % size]
	mov w4, wzr              // w4 = i (loop counter)
	
.Lrot_rotate_loop:
	cmp w4, w2
	b.ge .Lrot_done
	
	// Calculate src_index = (i + rot) % size
	add w5, w4, w3           // w5 = i + rot
	udiv w6, w5, w2          // w6 = (i + rot) / size
	msub w5, w6, w2, w5      // w5 = (i + rot) - (w6 * size) = (i + rot) % size
	
	// pattern[i] = tmp[src_index]
	ldrb w6, [x1, w5, uxtw]  // Load tmp[src_index]
	strb w6, [x0, w4, uxtw]  // Store to pattern[i]
	
	add w4, w4, #1           // i++
	b .Lrot_rotate_loop
	
.Lrot_done:
	ret 

/*
 * generator_build_events_asm - Pre-compute entire event queue in assembly
 * ----------------------------------------------------------------------
 * void generator_build_events_asm(event_queue_t *q, rng_t *rng, 
 *                                  const uint8_t *kick_pat, const uint8_t *snare_pat, const uint8_t *hat_pat,
 *                                  uint32_t step_samples);
 *
 * Converts the event queue building loop from C to assembly for ultimate performance.
 * This is the final orchestration step - building the complete musical timeline.
 *
 * Event generation rules:
 * - Drums: kick/snare/hat based on euclidean patterns
 * - Melody: triggers at specific bar positions (0, 8, 16, 24) 
 * - Mid: stochastic triggers with 10% probability on certain beats
 * - Bass: triggers at beginning of each bar (step 0)
 *
 * Constants:
 * - TOTAL_STEPS = 32, STEPS_PER_BAR = 16
 * - Event types: KICK=0, SNARE=1, HAT=2, MELODY=3, MID=4, FM_BASS=5
 */

	.globl _generator_build_events_asm

_generator_build_events_asm:
	// Arguments: x0=q, x1=rng, x2=kick_pat, x3=snare_pat, x4=hat_pat, w5=step_samples
	
	// Save callee-saved registers
	stp x19, x20, [sp, #-80]!
	stp x21, x22, [sp, #16]
	stp x23, x24, [sp, #32]
	stp x25, x26, [sp, #48]
	stp x27, x28, [sp, #64]
	
	// Initialize event queue: q->count = 0
	str wzr, [x0, #4096]        // q->count = 0 (events array is 4096 bytes)
	
	// Register assignments for loop
	mov x19, x0                 // x19 = q (event queue)
	mov x20, x1                 // x20 = rng
	mov x21, x2                 // x21 = kick_pat
	mov x22, x3                 // x22 = snare_pat
	mov x23, x4                 // x23 = hat_pat
	mov w24, w5                 // w24 = step_samples
	mov w25, wzr                // w25 = step (loop counter)
	
	// Constants
	mov w26, #32                // w26 = TOTAL_STEPS
	mov w27, #16                // w27 = STEPS_PER_BAR
	
	// RNG_FLOAT constants - removed unused constant
	
.Lbuild_loop:
	// Check loop condition: step < TOTAL_STEPS
	cmp w25, w26
	b.ge .Lbuild_done
	
	// Calculate t = step * step_samples
	mul w6, w25, w24            // w6 = t = step * step_samples
	
	// Calculate bar_step = step % STEPS_PER_BAR
	udiv w7, w25, w27           // w7 = step / STEPS_PER_BAR
	msub w8, w7, w27, w25       // w8 = bar_step = step - (w7 * STEPS_PER_BAR)
	
	// Check kick pattern: if(kick_pat[step % STEPS_PER_BAR])
	ldrb w9, [x21, w8, uxtw]    // w9 = kick_pat[bar_step]
	cbz w9, .Lcheck_snare
	
	// Push kick event: eq_push(q, t, EVT_KICK, 0)
	mov w10, #0                 // EVT_KICK = 0
	mov w11, #0                 // aux = 0
	bl _generator_eq_push_helper_asm
	
.Lcheck_snare:
	// Check snare pattern: if(snare_pat[step % STEPS_PER_BAR])
	ldrb w9, [x22, w8, uxtw]    // w9 = snare_pat[bar_step]
	cbz w9, .Lcheck_hat
	
	// Push snare event: eq_push(q, t, EVT_SNARE, 0)
	mov w10, #1                 // EVT_SNARE = 1
	mov w11, #0                 // aux = 0
	bl _generator_eq_push_helper_asm
	
.Lcheck_hat:
	// Check hat pattern: if(hat_pat[step % STEPS_PER_BAR])
	ldrb w9, [x23, w8, uxtw]    // w9 = hat_pat[bar_step]
	cbz w9, .Lcheck_melody
	
	// Push hat event: eq_push(q, t, EVT_HAT, 0)
	mov w10, #2                 // EVT_HAT = 2
	mov w11, #0                 // aux = 0
	bl _generator_eq_push_helper_asm
	
.Lcheck_melody:
	// Check melody triggers: if(bar_step==0 || bar_step==8 || bar_step==16 || bar_step==24)
	cbz w8, .Lmelody_trigger    // bar_step == 0
	cmp w8, #8
	b.eq .Lmelody_trigger
	cmp w8, #16
	b.eq .Lmelody_trigger
	cmp w8, #24
	b.eq .Lmelody_trigger
	b .Lcheck_mid
	
.Lmelody_trigger:
	// Push melody event: eq_push(q, t, EVT_MELODY, bar_step/8)
	lsr w11, w8, #3             // w11 = aux = bar_step / 8
	mov w10, #3                 // EVT_MELODY = 3
	bl _generator_eq_push_helper_asm
	
.Lcheck_mid:
	// Check mid triggers: if(bar_step % 4 == 2 || ((bar_step%4==1 || bar_step%4==3) && RNG_FLOAT < 0.1))
	and w9, w8, #3              // w9 = bar_step % 4
	cmp w9, #2
	b.eq .Lmid_trigger          // bar_step % 4 == 2
	
	// Check if bar_step % 4 == 1 or 3
	cmp w9, #1
	b.eq .Lmid_rng_check
	cmp w9, #3
	b.ne .Lcheck_bass
	
.Lmid_rng_check:
	// Generate RNG_FLOAT and compare with 0.1
	bl _generator_rng_next_float_asm   // Returns float in s0
	
	// Compare with 0.1f
	mov w12, #0x3dcc
	movk w12, #0xcccd, lsl #16  // 0.1f in IEEE 754
	fmov s1, w12
	fcmp s0, s1
	b.ge .Lcheck_bass           // if RNG_FLOAT >= 0.1, skip
	
.Lmid_trigger:
	// Generate random aux value: rng_next_u32() % 7
	bl _generator_rng_next_u32_asm   // Returns uint32_t in w0
	mov w12, #7
	udiv w13, w0, w12
	msub w11, w13, w12, w0      // w11 = aux = w0 % 7
	
	// Push mid event: eq_push(q, t, EVT_MID, aux)
	mov w10, #4                 // EVT_MID = 4
	bl _generator_eq_push_helper_asm
	
.Lcheck_bass:
	// Check bass trigger: if(bar_step == 0)
	cbnz w8, .Lloop_next
	
	// Push bass event: eq_push(q, t, EVT_FM_BASS, 0)
	mov w10, #5                 // EVT_FM_BASS = 5
	mov w11, #0                 // aux = 0
	bl _generator_eq_push_helper_asm
	
.Lloop_next:
	// Increment step and continue loop
	add w25, w25, #1
	b .Lbuild_loop
	
.Lbuild_done:
	// Restore callee-saved registers
	ldp x27, x28, [sp, #64]
	ldp x25, x26, [sp, #48]
	ldp x23, x24, [sp, #32]
	ldp x21, x22, [sp, #16]
	ldp x19, x20, [sp], #80
	ret

/*
 * Helper function: eq_push equivalent
 * Inputs: w6=time, w10=type, w11=aux
 * Uses: x19=q
 */
	.globl _generator_eq_push_helper_asm
_generator_eq_push_helper_asm:
	// Load current count
	ldr w12, [x19, #4096]       // w12 = q->count
	
	// Check if count < MAX_EVENTS (512)
	cmp w12, #512
	b.ge .Leq_push_ret          // Skip if queue full
	
	// Calculate event address: &q->events[count]
	mov w13, #8                 // sizeof(event_t) = 8 bytes
	mul w14, w12, w13           // w14 = count * sizeof(event_t)
	add x15, x19, w14, uxtw     // x15 = &q->events[count]
	
	// Store event: {time, type, aux, padding}
	str w6, [x15]               // event.time = time
	strb w10, [x15, #4]         // event.type = type
	strb w11, [x15, #5]         // event.aux = aux
	
	// Increment count
	add w12, w12, #1
	str w12, [x19, #4096]       // q->count++
	
.Leq_push_ret:
	ret

/*
 * Helper function: rng_next_u32 equivalent
 * Inputs: x20=rng
 * Returns: w0=random uint32_t
 * Preserves: x20 (rng pointer)
 */
	.globl _generator_rng_next_u32_asm  
_generator_rng_next_u32_asm:
	// Save link register and preserve registers
	stp x29, x30, [sp, #-16]!
	
	// Implementation of SplitMix64 algorithm
	// uint64_t z = (r->state += 0x9E3779B97F4A7C15ULL);
	ldr x0, [x20]               // x0 = rng->state
	movz x1, #0x7C15, lsl #0
	movk x1, #0x7F4A, lsl #16
	movk x1, #0xB979, lsl #32
	movk x1, #0x9E37, lsl #48   // x1 = 0x9E3779B97F4A7C15
	add x0, x0, x1              // x0 = state + increment
	str x0, [x20]               // rng->state = new state
	
	// z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	lsr x1, x0, #30
	eor x0, x0, x1
	movz x1, #0xE5B9, lsl #0
	movk x1, #0x1CE4, lsl #16
	movk x1, #0x476D, lsl #32
	movk x1, #0xBF58, lsl #48   // x1 = 0xBF58476D1CE4E5B9
	mul x0, x0, x1
	
	// z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	lsr x1, x0, #27
	eor x0, x0, x1
	movz x1, #0x11EB, lsl #0
	movk x1, #0x3311, lsl #16
	movk x1, #0x49BB, lsl #32
	movk x1, #0x94D0, lsl #48   // x1 = 0x94D049BB133111EB
	mul x0, x0, x1
	
	// return z ^ (z >> 31);
	lsr x1, x0, #31
	eor x0, x0, x1
	
	// Return lower 32 bits
	mov w0, w0
	
	// Restore and return
	ldp x29, x30, [sp], #16
	ret

/*
 * Helper function: rng_next_float equivalent  
 * Inputs: x20=rng
 * Returns: s0=random float [0,1)
 */
	.globl _generator_rng_next_float_asm
_generator_rng_next_float_asm:
	// Save link register and floating-point context
	stp x29, x30, [sp, #-16]!
	
	// Call rng_next_u32
	bl _generator_rng_next_u32_asm   // w0 = random uint32_t
	
	// Implement: (rng_next_u32(r) >> 8) * (1.0f / 16777216.0f)
	lsr w0, w0, #8              // w0 = w0 >> 8
	ucvtf s0, w0                // s0 = (float)w0
	
	// Multiply by 1.0f / 16777216.0f = 5.960464477539063e-08
	movz w1, #0x0000, lsl #0
	movk w1, #0x3380, lsl #16   // IEEE 754 representation of 1.0f/16777216.0f
	fmov s1, w1
	fmul s0, s0, s1             // s0 = s0 * (1.0f/16777216.0f)
	
	// Restore and return
	ldp x29, x30, [sp], #16
	ret 

.section __TEXT,__cstring
.Ldbg_fmt:
	.asciz "ASM: rem=%u proc=%u pos=%u\n"
.Ldbg_pre_fmt:
	.asciz "PRE: rem=%u step=%u pos=%u\n"
.Ldbg_post1_fmt:
    .asciz "P1:  rem=%u proc=%u pos=%u\n" 
.Ldbg_rms_fmt:
	.asciz "RMSraw=%u\n" 
.Ldbg_scratch_fmt:
	.asciz "SCR drums=%u synth=%u n=%u\n" 