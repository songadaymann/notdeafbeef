# Generator Debugging Log  (Slice-4 → Slice-6 transition)

> 2025-H2 migration from mixed C/ASM to full-ASM generator (`generator.s`).

## Context
* Goal: run `segment` with the work-in-progress assembly generator plus existing ASM voices and C delay/limiter.
* Initial symptom: binary ran but wrote **0 frames**; when built with `VOICE_ASM="GENERATOR_ASM DELAY_ASM"` it appeared to hang.

## Session timeline
| Step | Command / Change | Observed effect | Notes |
|----|----|----|----|
| 1 |`make -C src/c segment USE_ASM=1 VOICE_ASM="GENERATOR_ASM DELAY_ASM"`| Printed two DEBUG lines then looped forever; no WAV| suspected zero `step_samples` or bad pointer |
| 2 | Added debug `printf` inside `_generator_process` (printing `frames_rem`, `frames_to_process`, `pos_in_step`) | Flood of `proc=0` lines – confirmed loop never advanced | indicates `frames_to_process` always 0 |
| 3 | LLDB breakpoint at C `generator_process`, examined `g.mt.step_samples` | value **non-zero** (≈12 713) before call into ASM | timing info written correctly by C init |
| 4 | Examined same field inside ASM (`x24` pointer) | ASM saw **0** ⇒ wrong pointer or macro side-effect |
| 5 | Realised `generator.c` was compiled with `-DGENERATOR_ASM`; huge `#ifndef` blocks skipped timing setup when that macro is defined | C still built but produced blank timing fields when macro set |
| 6 | Added Makefile rule compiling `generator.c` **without** `-DGENERATOR_ASM` | Loop progressed, but link now failed with duplicate symbols for voices and delay | need to exclude corresponding C objects when ASM present |
| 7 | Drafted Makefile logic: rebuild `GEN_OBJ` conditionally – include C fallback only if its ASM twin is absent; always include `generator.o` for `generator_init` | Build now links when using single voice, but revealed more duplicate/undefined symbol combinations when multiple voice flags combined | iterative Makefile cleanup pending |
| 8 | Reworked Makefile: per-voice C objs always included but processes compiled out via `#ifndef *_ASM`; special case for `generator.c` removed; duplicate symbol link clean | Build succeeds for `GENERATOR_ASM + DELAY_ASM`; binary runs | still stalls after first delay_init print |
| 9 | Observed runtime hang: no further prints after delay_init → suspect infinite loop inside `_generator_process` | Likely `frames_to_process==0` loop bug (Slice-4 math) | need deeper logging |

## Updated Findings
5. Link layer now stable – duplicates fixed, C init runs with valid timing fields.
6. Stall occurs inside the ASM frame loop; most probable cause is `frames_to_process` occasionally evaluating to 0, so the outer `while(frames_rem)` never progresses.

## Next actions (rev 2)
1. Temporarily **restore the small debug printf** block in `generator.s` (lines around Slice-4) to print `frames_rem`, `frames_to_process`, and `pos_in_step` each iteration.
2. Re-run with a small buffer (e.g. `tests/minimal_kick_test`) to capture the first ~20 iterations; confirm whether `proc=0` appears repeatedly.
3. If `proc=0` repeats, inspect the values of `step_samples` (w9) and `pos_in_step` (w8) in LLDB to see which operand is wrong.
4. Fix the arithmetic so `frames_to_process` is **always ≥1** when `frames_rem>0`.
5. Once the loop completes correctly, remove the debug prints again and proceed to enable `LIMITER_ASM` (Slice-6 target).

## Round 3 – Pointer-clobber Bus-Error (Jul 2025)

| Step | Change / Investigation | Result | Insight |
|----|----|----|----|
| 10 | Wrapped debug-`printf` with register save/restore (`x8`,`x11` etc.) | Frame loop now advances; program segfaults in `generator_mix_buffers_asm` (first `st1`) | L/R dest pointer (`x1`) became **NULL** between voice processing and mixer |
| 11 | LLDB breakpoints before mixer entry – print args | Pointer already corrupt **before** mixer; corruption happens earlier |
| 12 | Added breakpoint at scratch-setup (0x100000F90); register dump | `x13-x16` (scratch ptrs) held garbage values `0x3ff4…`; their bases (`x25-x28`) valid; `x12` offset = 0 | Scratch start overwritten right after calculation |
| 13 | Identified culprit: immediately after computing scratch ptrs we executed `stp x25,x26,[sp,#-0x10]!` etc.  Because `x25 == sp`, those pushes landed **inside the scratch buffer**, clobbering first 32 bytes. | Voices later read those floats as pointers → Bus error inside `snare_process`. |
| 14 | Temporary test: removed pushes/pops of `x25-x28` → still crash | Any push after `x25=sp` (even w11 save) still overwrites scratch. Need temp area **before** setting `x25`. |
| 15 | Final fix plan: 1) `sub sp,#16` reserve slot for `w11` **before** `x25 = sp`; 2) store/restore `w11` in that slot; 3) recompute scratch ptrs after C call. | This keeps bookkeeping below scratch; no overwrite expected – ready to implement. |

## Round 4 – Offset-hunting & First Audible Hit (6 Jul 2025)

| Step | Change / Investigation | Result | Insight |
|----|----|----|----|
| 16 | Added **PRE** printf at the top of `.Lgp_loop` to dump `rem`, `step_samples`, `pos_in_step` | Printed enormous `step` (≈45 M) showing we still read the wrong field | Confirms `ldr w9,[x24,#3]` (word 3) is wrong offset |
| 17 | Tried byte offsets **12** and **20** for `step_samples` loads | PRE still garbage; loop never advances | music_time_t not at struct head – need accurate offsetof |
| 18 | Realised a proper C layout dump is needed (clang record-layout / offsetof) instead of guess-and-check | TODO | Will calculate exact byte offset for `step_samples` then patch both `ldr` sites |
| 19 | Despite wrong offset we lowered scratch overwrite risk by saving `w11` in **x22** (callee-saved) instead of stack | Bus-error location unchanged, confirming mix write crash stems from over-large `proc` count, not w11 save | |

Current status: generator plays **one simultaneous hit** (first audible render 🎉) but outer loop still mis-reads `step_samples`, so only step 0 is processed; subsequent pointer math overruns buffers → Bus error in vector mixer.

Next action: generate exact field offsets with a one-off C program or `clang -fdump-record-layout`, update `generator.s`, verify PRE shows `step≈12 700`, restore stack save for `w11`, then re-enable delay/limiter. 

## Round 5 – Heap-Scratch Refactor & Counter Corruption (7 Jul 2025)

| Step | Change / Investigation | Result | Insight |
|----|----|----|----|
| 20 | Replaced giant **stack** scratch block with `malloc`/`free` heap allocation; refactored pointer setup | Segment now renders end-to-end without EXC_BAD_ACCESS or bus errors | Stack frame no longer collides with `generator_t`; memory stable |
| 21 | Added robust `x10 = g+0x1100` recomputes at loop top **and** step-boundary path | No more null/0x2 pointer derefs | Caller-saved `x10` still clobbered by helpers, so we regenerate instead of preserving |
| 22 | Wrapped initial `_generator_clear_buffers_asm` call with `stp/ldp x21,x22` | `frames_rem` survives clear-buffers helper | Helper wasn't preserving x21 (caller-saved) |
| 23 | Removed early `memset(L/R)` calls; rely solely on scratch zero-clear | Eliminated redundant libc calls | Keeps prologue simpler; avoids extra register churn |
| 24 | **Current status:** Program runs; writes `segment.wav` but file is **all zeros**. PRE print shows wildly corrupted `frames_rem`, `step_samples`, `pos_in_step` before first voice call. | Silent output proves counters still clobbered post-setup. Next step: LLDB watch-points to find culprit writes. |

Current status: generator plays **one simultaneous hit** (first audible render 🎉) but outer loop still mis-reads `step_samples`, so only step 0 is processed; subsequent pointer math overruns buffers → Bus error in vector mixer.

Next action: generate exact field offsets with a one-off C program or `clang -fdump-record-layout`, update `generator.s`, verify PRE shows `step≈12 700`, restore stack save for `w11`, then re-enable delay/limiter. 

## Round 6 – Voice-Helper Register Clobber (8 Jul 2025)

| Step | Change / Investigation | Result | Insight |
|----|----|----|----|
| 25 | Added `TRACE1` printf **after** `_generator_process_voices` (P1 line) | PRE prints garbage counters, P1 shows **frames_rem/proc** already corrupted → helper overwrote them | Corruption happens inside or right after voice helper |
| 26 | Wrapped voice call with `stp/ldp x21,x22` and re-loaded `pos_in_step` | Build succeeded but program **seg-faults** before P1; PRE still garbage | Earlier prologue helper (`generator_clear_buffers_asm`) may now clobber x22 as well |

Next actions (rev 3):
1. Also preserve **x21 & x22** around `_generator_clear_buffers_asm` (currently only x21 safe).
2. Rebuild and verify PRE shows sane `frames_rem` and `step_samples`.
3. If still corrupt, step further back to scratch allocation and initial memset path to find first bad write. 