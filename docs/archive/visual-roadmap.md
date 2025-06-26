# Visual Re-implementation Road-map

> Goal: recreate the Python visual layer (euclid_delay_playground.py) entirely in portable C while staying light-weight, seed-reproducible, and tightly audio-synchronised.

---

## 0. Rendering back-end choice

| Requirement | Rationale |
|-------------|-----------|
| GPU-accelerated 2-D (textures, lines, filled polys) | Needed for particles, floor tiles, CRT FX |
| Easy pixel access or post-process shader | For scanlines, chromatic aberration, noise overlay |
| Cross-platform | Same binary on macOS / Linux / Windows |
| Minimal extra deps | Keep build fast & pure C |

**Recommendation:** SDL 2 for window/input/timing + OpenGL (or Metal on macOS). One dependency, available everywhere.

---

## 1. Incremental migration stages

### Stage 1 – skeleton window & fixed-rate loop
* New `video.c/.h` → create SDL window + GL context.
* Expose `video_begin_frame() / video_end_frame()` at target **FPS = 30**.
* Main thread: poll events, call begin/ end once per frame.

### Stage 2 – basic draw primitives
Implement immediate-mode helpers (in `draw.c`):
* Filled circle (orbiting circle)
* Thick outlined circle ("thick" / "rings")
* Regular polygon (poly mode)
* Per-pixel plot (Lissajous figure)

### Stage 3 – RMS modulation
* Inside `generator_process()` accumulate RMS per 512-frame audio block.
* Write to a lock-free float slot / ring-buffer.
* In visual loop read RMS → scale circle radius / colour.

### Stage 4 – scrolling "Metroid floor"
* Port Python `build_tile()` & `build_slope_tile()` – pre-generate 32×32 textures.
* Generate `terrain_pattern[]` in C (same random rules).
* Each frame draw the visible tiles with offset =`frame*SCROLL_SPEED % TILE_SIZE`.

### Stage 5 – particle engine
* `particle_t {x,y,vx,vy,life,texID}`.
* Spawn bursts on saw hits; update & draw every frame.
* Texture atlas with ASCII glyphs.

### Stage 6 – bass-hit procedural shapes
* Shapes: triangle, diamond, hexagon, star, square (triangle fans or indexed quads).
* `bass_hit_t` array; spawn every 2 bars, decay over time.

### Stage 7 – CRT / VHS degradation shader
Single full-screen fragment shader implementing:
* Scanlines (`SCANLINE_ALPHA`)
* Chromatic aberration (`CHROMA_SHIFT`)
* Colour bleed (`COLOR_BLEED`)
* Noise overlay (`NOISE_PIXELS`)
* Persistence: blend previous FBO with weight =`PERSISTENCE`
* Jitter & frame-drop (`JITTER_AMOUNT`, `FRAME_DROP_CHANCE`)

### Stage 8 – polish & seed parity
* Move all seed-driven RNG (base_hue, degrade params) to C.
* CLI flags (`--fps`, `--vsync`, `--noscanlines`, …).
* VSYNC toggle + frame-time budgeting.

---

## 2. File-level layout
```
include/
    video.h      SDL / GL façade
    draw.h       immediate-mode primitives
    particles.h  particle system API
    terrain.h    tile + terrain helpers
    crt_fx.h     post-process FBO + shader API
src/
    video.c draw.c particles.c terrain.c crt_fx.c
```
`main_realtime.c` initialises generator + video, then runs the visual loop.

`generator.c` gains an RMS meter → single-producer / single-consumer buffer.

---

## 3. Milestone checklist
- [x] **M1** window opens, black background, FPS locked
- [x] **M2** orbiting circle pulses with RMS
- [x] **M3** scrolling floor visible
- [x] **M4** particle bursts on beat
- [x] **M5** bass shapes rendered
- [x] **M6** CRT shader applied
- [x] **M7** seed reproducibility & CLI options

---

## 4. Build & dependency notes
macOS:
```bash
brew install sdl2
clang … $(sdl2-config --cflags --libs) -framework OpenGL
```
Linux:
```bash
sudo apt install libsdl2-dev
clang … -lSDL2 -lGL
```
Windows (MinGW):
```bash
clang … -lSDL2main -lSDL2 -lopengl32
```
No C++ required, stays pure-C. Hot paths can later be swapped for hand-written SIMD / assembly.

---

## 5. Host-layer & assembly strategy

To keep the eventual hand-written assembly path simple while still moving quickly on macOS, we'll separate the project into two layers:

```
include/
    host.h         // window + input + present; returns RGBA framebuffer ptr
    raster.h       // pure-C pixel pipeline; later replaced by SIMD/asm
src/
    host_sdl.c     // current implementation (SDL + texture blit), works macOS & Windows
    host_win32.c   // (optional) zero-dependency Win32 + GDI blitter
    raster.c       // circle, polygon, tile, particle drawing in C
    raster_asm.S   // (future) x86-64 / NEON assembly versions of the hot paths
```

Key rules:
1. **Raster code sees only** `uint32_t *fb`, `int width`, `int height`. No SDL, GL, or platform calls.
2. **Host code owns** window creation, frame pacing, and pushing the RGBA buffer to the screen.
3. When ready for assembly, swap `raster.c` functions with `raster_asm.S`—no other files change.

### Immediate steps
* Replace the SDL renderer calls with a software framebuffer path (SDL texture + `SDL_UpdateTexture`).
* Start `raster_clear()` and `raster_circle()` so M2 (orbiting circle) can run.
* Keep SDL for now; when moving to Windows/x86, you can either continue to use SDL or switch to `host_win32.c` without touching raster code.

This guarantees zero wasted work and an easy transition to hand-optimised assembly later.

---

Happy hacking! If you'd like me to scaffold Stage 1, just say the word.

---

## 6. Software-raster drawing primitives

| Primitive | Function | Status |
|-----------|----------|--------|
| Pixel plot | _internal_ `plot()` | ✔ (used by others) |
| Clear framebuffer | `raster_clear()` | ✔ |
| Outlined circle (variable thickness) | `raster_circle()` | ✔ |
| Filled circle | `raster_fill_circle()` | ☐ (TBD) |
| Horizontal / vertical line | `raster_line()` | ☐ |
| Regular polygon (filled / outline) | `raster_poly()` | ☐ |
| 32×32 RGBA tile blit | `raster_blit_rgba()` | ☐ |
| Alpha-blended blit | `raster_blit_rgba_alpha()` | ☐ |
| Star / hexagon / diamond helpers | built on `raster_poly` | ☐ |

> The primitives marked ✅ are implemented in `src/raster.c`. The others will be added as the corresponding milestones (floor tiles, particles, shapes) are tackled.

--- 