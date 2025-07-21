# Visual Development Plan - NotDeafBeef Project

## Audio System Checkpoint ✅

**Git Tag**: `audio-stable-v1` (commit `2189a42`)  
**Target Assembly Config**: `GENERATOR_ASM KICK_ASM SNARE_ASM HAT_ASM MELODY_ASM LIMITER_ASM`  
**Binary Checksum**: `184fe574874d48e9db9c363ea808071df6af73b72729b4f37bf772488867b765`  

**✅ Status**: Working assembly audio system with drums, melody, generator, and limiter. Multiple seeds tested successfully.  
**Note**: FM_VOICE_ASM excluded from stable build - register alias fixes need to be forward-ported to stable branch.

## Overview

After 54 rounds of debugging to achieve a fully working assembly FM synth, we're now ready to add visuals while protecting the audio system. This document outlines the three-stage approach: **Python → C → Assembly**.

## Hard Isolation Strategy 🛡️

**Core Principle**: Audio and visuals share **zero code**. They synchronize via **timecode only**.

### Audio Bridge Interface
```c
// include/audio_bridge.h - Minimal read-only interface
unsigned get_audio_time_ms(void);    // Current position in segment
float get_rms_level(int frame_idx);  // Pre-computed RMS for visual sizing
```

### Build Separation
- `make audio` - Assembles only audio objects
- `make vis` - Compiles C/ASM visuals  
- CI gate: audio binary SHA-256 must remain unchanged

## Stage A: Python → C Scaffolding

### 1. Lock Audio System 🔒
```bash
git tag audio-stable-v1
make -C src/c clean  
make -C src/c segment USE_ASM=1 VOICE_ASM="GENERATOR_ASM KICK_ASM SNARE_ASM HAT_ASM MELODY_ASM FM_VOICE_ASM LIMITER_ASM"
sha256sum bin/segment > audio-checksum.txt
```

### 2. Technology Stack
- **Rendering**: SDL2 (similar to Pygame API, WASM-ready for onchain)
- **Bridge**: Minimal C shim DLL exposing audio timing
- **Verification**: Python reference + ImageMagick `compare` for pixel-perfect porting

### 3. Verification Strategy
- Run Python in one window, capture screenshots every 2 seconds
- Run C in another window, image-diff to prove visual equivalence
- Use identical RNG seeds and constants (WIDTH=512, HEIGHT=512, FPS=30)

## Stage B: C Feature Port

### Feature Implementation Order
1. **Window + Basic Shapes** (SDL2 basic drawing)
2. **Terrain System** (RGBA `uint32_t` tile buffers replacing Pygame surfaces)  
3. **Particle Engine** (Fixed 256-particle pool, no malloc)
4. **Bass Hit Shapes** (Separate pool, pre-computed sin/cos tables)
5. **Post-Processing** (persistence, scanlines, chroma shift, blur, jitter, noise)
6. **Audio Sync** (RMS-driven sizing, step-based explosions)

### Visual System Components (from Python reference)

#### Core Visual Elements
- **Orbiting centerpiece**: 4 modes based on BPM
  - `thick`: Simple thick circle outline (BPM < 70)
  - `rings`: Multi-layered rings (70-100 BPM)  
  - `poly`: Rotating polygon (100-130 BPM)
  - `lissa`: Lissajous figure-8 (130+ BPM)

#### Procedural Terrain
- **Metroid-style scrolling floor**: 32×32 tile system
- **Deterministic generation**: Seed-driven patterns
- **Tile types**: flat, wall, slope_up, slope_down, gap
- **Rock texture**: Procedural 3-level shading

#### Audio-Reactive Elements  
- **RMS-driven sizing**: Circle radius = `30 + 80 * level`
- **Particle explosions**: On saw melody steps (0,8,16,24)
- **Bass hit shapes**: Every 8 beats, large geometric animations
- **Step synchronization**: `(elapsed_ms/1000)/step_sec`

#### Post-Processing Effects (Seed-Randomized)
- **Persistence**: Ghost trails (0.3-0.9 alpha)
- **Scanlines**: Retro effect (0-200 intensity)  
- **Chromatic aberration**: RGB channel shift (0-5 pixels)
- **Noise**: Random pixel scatter (0-300 pixels)
- **Jitter**: Screen shake (0-3 pixel offset)
- **Frame drops**: Repeat frame effect (0-10% chance)
- **Color bleed**: Horizontal blur (0-0.3 amount)

## Stage C: Assembly Optimization & Onchain Prep

### 1. Fixed-Point Conversion
- Replace floats with 16.16 fixed-point for angles, positions, alpha
- Maintain compile-time switch for float vs fixed comparison

### 2. Selective Assembly Port
- Move hot inner loops to individual `.s` files **only when necessary**:
  - Blitting operations
  - Circle/polygon rasterization  
  - Scanline & chroma passes
- Keep rest in readable C

### 3. Determinism Audit
- All random draws from seeded RNG
- Remove `time()` calls except audio clock
- Avoid undefined behavior (signed shifts, pointer aliasing)
- Ensure bit-perfect results across compilers

### 4. Onchain Structure
- **Contract storage**: Master seed + visual parameters (12-16 bytes)
- **Pure function**: `(seed, t) → frame_buffer_hash`
- **Off-chain rendering**: C/ASM as reference implementation
- **Compression**: L2-friendly drawing commands vs full pixel data

## Implementation Checklist

### Phase 1: Foundation
- [ ] Create `include/audio_bridge.h` with timing callbacks
- [ ] Skeleton `vis_main.c` - SDL2 window + 30 FPS clear
- [ ] Copy orbiting circle math, verify RMS-driven sizing
- [ ] Port terrain tile builder to `tile.c`, dump PNG for inspection

### Phase 2: Visual Systems  
- [ ] Fixed-pool particle engine (256 particles)
- [ ] Bass hit shape animations (geometric primitives)
- [ ] Post-processing chain (profile ~1.5-2ms/frame at 512²)

### Phase 3: Integration & Verification
- [ ] Seed fuzz testing (5000 runs)
- [ ] Pixel checksum verification vs Python (first 100 frames)
- [ ] Audio hash verification (unchanged)
- [ ] Tag milestone `vis_c_v1`

## Risk Mitigation

### Audio Protection
- **Zero shared code** between audio and visual systems
- **Read-only timecode** synchronization  
- **Separate build targets** with independent compilation
- **SHA-256 verification** of audio binary integrity

### Visual Quality Assurance
- **Pixel-perfect verification** against Python reference
- **Deterministic RNG** for reproducible results
- **Seed-based testing** for comprehensive coverage

## Success Criteria

1. **Audio System**: Remains completely untouched and verified by checksum
2. **Visual Fidelity**: Pixel-perfect match to Python reference for key seeds
3. **Performance**: <2ms render time at 512² resolution  
4. **Determinism**: Identical output across builds and platforms
5. **Onchain Ready**: Clean separation of deterministic core vs rendering implementation

---

*"One-upping Deafbeef": From C-only to full Assembly implementation* 🎵✨
