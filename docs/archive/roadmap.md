# Euclid-Delay Refactor Roadmap

**Goal**: Translate `euclid_delay_playground.py` into plain C (no external libraries) on macOS (Apple Silicon).  Visuals will be tackled after audio.  Ultimately every C module should be replaceable with hand-rolled assembly.

---

## Guiding Principles

* No runtime dependencies – only the C standard library.
* Deterministic, seed-driven output identical across builds.
* Keep all memory static/stack-allocated so the same code can drop into asm later.
* Organise code as small, pure functions that map directly to DSP/visual primitives.

---

## Phase 0 — Scaffolding (½ day)

- [x] Create minimal project structure (`src/`, `include/`, `Makefile`)
- [x] Provide `main.c` that just prints *"build ok"*.
- [x] Add a tiny `wav_writer.c` (RIFF header + 16-bit PCM) – this will be our first I/O route.

---

## Phase 1 — Core Utilities (1 day)

- [x] Implement seedable RNG (`rand.h` with SplitMix64 inlines).
- [x] Port `euclidean()` rhythm generator (`euclid_pattern`).
- [x] Skip colour math for now (audio-only).

Deliverables:
* `rand.h/.c` – seedable RNG returning `float in [0,1)` and 32-bit ints.
* `euclid.h/.c` – algorithm producing step arrays.

---

## Phase 2 — DSP Building Blocks (2 days)

- [x] Block-based sine oscillator (`osc.c`, `osc.h`) and WAV test.
- [x] Saw, triangle, square oscillators.
- [x] White-noise generator.
- [x] Exponential envelope helper.
- [x] Stereo delay line.

All functions operate on slices: `void render(float *L,float *R,int n, Params *p)` so they chain easily.

---

## Phase 3 — Voice Implementations (3 days)

| Instrument | Status |
|------------|--------|
| Kick       | ✅ implemented (`kick.c`, `gen_kick`) |
| Snare      | ✅ implemented (`snare.c`, `gen_snare`) |
| Hat        | ✅ implemented (`hat.c`, `gen_hat`) |
| Melody Saw | ✅ implemented (`melody.c`, `gen_melody`) |
| Mid-Notes FM | ✅ implemented (`fm_voice.c`, `gen_fm`) |
| Bass FM    | ✅ implemented (in same) |

Each voice exposes `trigger/process` functions.

---

## Phase 4 — Segment Composer (2 days)

1. Re-create timing constants (`SR`, `BPM`, `step_sec`, …).
2. On program start, pre-compute patterns using `euclid()`.
3. ✅ Event-driven renderer:
   * Steps are pre-scanned and `Event` objects pushed into a queue.
   * The renderer pops queued events and calls the appropriate `trigger()` functions.
4. Write final buffer to `out.wav` via `wav_writer`.

Success criterion: resulting WAV is playable with `afplay out.wav` and sounds roughly like the Python original.

---

## Phase 5 — Real-Time Audio (optional, 2 days)

If immediate playback is desired before assembly stage:

* macOS CoreAudio AudioQueue minimal wrapper `coreaudio.c`. _Still zero outside deps._
* Stream the buffer in 256-sample chunks while the composer fills ahead-of-time.

(Skip if WAV-file output is adequate.)

---

## Phase 6 — Determinism & CLI (½ day)

* Implement `--seed 0xNN` arg parsing (use `strtoull`). ✅
* Unit-test that identical seeds produce identical WAV hashes.
* Seed should influence musical variation (pattern fills, preset selection, etc.) so each seed sounds distinct while remaining deterministic.
  _Status: complete — all Python audio parameters are now seed-driven in C._

  Key deterministic features implemented:
  * **Mid-Notes Voice:** Waveform roulette (tri, sine, square, 4x FM) chosen by seed.
  * **Bass FM:** Three profiles (`default`, `quantum`, `plucky`) chosen by seed.
  * **Melody Saw:** Cubic soft-clip waveshaper (`1.5x − 0.5x³`) to match timbre.
  * **Core Parameters:** BPM, delay time, root key, scale type, and Euclid pattern rotation derived from seed.
  * **Limiter:** A simple soft-knee limiter on the master bus now prevents clipping.

---

## Phase 7 — Performance Polish (1 day)

* Remove `malloc`; switch to fixed-size static arrays.
* Replace `float` math with `float32_t` and, where easy, fixed-point to prepare for asm.
* Inline hot DSP loops; ensure no function pointers in critical path.

---

## Phase 8 — Handoff to Assembly (open-ended)

* For each `.c` listed in Phase 3, write an equivalent `.s` file using ARM64 NEON where beneficial (kick, snare, delay).
* Provide compile switch `USE_ASM` to pick either C or asm implementation.

---

## Phase 9 — Visuals (future)

Audio complete ⇒ revisit the Python visuals.  Options:

1. **C + SDL** (one dependency) while keeping asm core; OR
2. **Bare-metal Metal** via Objective-C minimal wrapper; OR
3. Skip visuals and embed resulting WAV in Deafbeef-style NFT metadata.

We will decide once the audio path is frozen.

---

## Rough Timeline

*Week 1*: Phase 0-2  
*Week 2*: Phase 3-4  
*Week 3*: Polish, determinism, optional real-time, begin asm.

(*Visuals and NFT minting come after proof-of-sound.*)