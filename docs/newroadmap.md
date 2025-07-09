# NotDeafbeef • Pure-Assembly Roadmap 2025-H2

## 1  Where We Stand

✔ Core DSP helpers in hand-written AArch64 assembly – oscillators, Euclid, Noise, Delay, Limiter.

✔ Percussive + tonal voices in assembly – Kick, Snare, Hat, Melody – fully verified, bit-identical to C, no stack corruption.

✔ Mixed build (`USE_ASM=1` + C generator) still renders `segment.wav` cleanly.

✔ `generator.s` Slices 0 → 2 implemented and build successfully; silent render proves stack-safe outer loop.  (Opt-in via `VOICE_ASM="GENERATOR_ASM"`).

✔ 2025-07-05 milestone: Slice 4 assembly generator produced its **first audible render** (all voices fire, single simultaneous hit) proving voice DSP + NEON mixer path work end-to-end.
⚠ Outer loop still broken – wrong struct offsets make `step_samples` read as whole-segment length, so only the very first step renders and pointer math later overflows, crashing in `_generator_mix_buffers_asm`. Debugging focuses on fixing offsets & frame counters before re-enabling delay/limiter.

---

## 2  Goal

Port `