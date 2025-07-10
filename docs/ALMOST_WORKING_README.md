# "Almost-Working" All-ASM Build (Nov 2025)

This branch captures a **fully functional** render path where every realtime DSP component has been ported to ARM64 assembly **except** the FM voices (bass / pad) and the melody voice, which still run in their C / NEON reference implementations.

## What’s running in Assembly

* `generator.s` – outer process loop, scratch-buffer handling, NEON mixer, RMS helpers.
* `kick.s`, `snare.s`, `hat.s` – drum voices.
* `delay.s` – additive ping-pong delay (L→R, R→L) **NEW in this snapshot**.
* `limiter.s` – look-ahead hard limiter.
* Support helpers that were already in assembly (`osc_sine.s`, `osc_shapes.s`, `noise.s`, `euclid.s`, etc.).

## Still in C / NEON

* `fm_voice_process` ( `fm_voice.c` + `fm_voice_neon.c` ) – bass & pad.
* `melody_process` (`melody.c`) – saw-lead.
* `simple_voice.c` – utility for certain synth parts.
* All non-DSP infrastructure: `generator_init`, event queue, WAV writer, CoreAudio glue, etc.

## How to Build & Render

```bash
# From repository root
make -C src/c segment USE_ASM=1 \
    VOICE_ASM="GENERATOR_ASM KICK_ASM SNARE_ASM HAT_ASM DELAY_ASM LIMITER_ASM"
```

This produces `bin/segment` (CLI) and will render two WAVs when executed:

* `segment.wav` – the main output (≈ 9.3 s, 52.03 BPM)
* `seed_0xcafebabe.wav` – deterministic reference render used in tests

You should hear:

* Dry drums (kick / snare / hat)
* Melody lead
* FM bass
* Delay tail (stereo echoes)
* Overall loudness capped by the hard-limiter

⚠️  **Known issue:** the mid-range FM pad (processed by `fm_voice_process`) is still **silent** in this snapshot.  All nine EVT_MID triggers fire and the C implementation executes, but the rendered audio is zero.  Investigation continues (suspect register clobber or buffer overwrite after mixing).

If you hear only the delay tail and not the dry mix, verify that you built with the additive assembly delay (`DELAY_ASM`) enabled.

---

### Current Focus

* Debug why FM pad is silent despite triggers firing.
* Ensure slice-shortening logic in `generator.s` matches the C fallback, then re-enable FM voices in assembly once stable.

### Next Milestones

1. Port melody voice to assembly and resolve the current duplicate-symbol collision when `MELODY_ASM` is enabled.
2. Rewrite FM voices in ARM64/NEON for a truly 100 % assembly signal chain. 