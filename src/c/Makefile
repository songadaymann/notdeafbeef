# Simple build script – Phase 0 scaffolding

CC := clang
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

# Determine host arch first
ARCH := $(shell uname -m)

# Detect request for cross-compilation (set CROSS=1 from CLI)
ifndef CROSS
CROSS := 0
endif

# If we are on an x86_64 host *and* not explicitly cross-compiling, disable ASM
ifeq ($(ARCH),x86_64)
  ifeq ($(CROSS),0)
    USE_ASM := 0
  endif
endif

# If CROSS=1, enforce arm64 code-gen flags
ifeq ($(CROSS),1)
  USE_ASM := 1
  CFLAGS += -target arm64-apple-macos13
endif

# Default USE_ASM detection if not already set
ifndef USE_ASM
  # Simple architecture detection - on ARM64 machines, enable ASM by default
  ifeq ($(ARCH),arm64)
  USE_ASM := 1
  else
  USE_ASM := 0
  endif
endif



# base flags
CFLAGS := -std=c11 -Wall -Wextra -O2 -Iinclude $(SDL_CFLAGS)

# float32_t helper macro (typedef-like).  The NEON compile unit undefines it to avoid clash.
CFLAGS += -Dfloat32_t=float

# Append profiling flags when PROFILE=1
ifeq ($(PROFILE),1)
CFLAGS += -pg -fno-omit-frame-pointer
endif

LDFLAGS := -framework AudioToolbox -framework CoreFoundation -framework OpenGL $(SDL_LIBS)

# BEGIN ASM SUPPORT
ifeq ($(USE_ASM),1)
ASM_DIR := ../asm/active
ASM_SRC := $(wildcard $(ASM_DIR)/*.s)
# fm_voice.s is now implemented – include it in the build
#ASM_SRC := $(filter-out $(ASM_DIR)/fm_voice.s,$(ASM_SRC))
ASM_OBJ := $(ASM_SRC:.s=.o)
# per-module ASM optimisation flags
CFLAGS += -DOSC_SINE_ASM -DOSC_SHAPES_ASM
# Voice assembly functions still have register corruption issues when combined
# CFLAGS += -DKICK_ASM -DSNARE_ASM -DHAT_ASM -DMELODY_ASM -DFM_VOICE_ASM
else
ASM_DIR :=
ASM_SRC :=
ASM_OBJ :=
endif
# END ASM SUPPORT

ifeq ($(USE_ASM),1)
  ifneq ($(ARCH),arm64)
    # x86 hosts need explicit cross-target so clang understands AArch64 mnemonics
    CFLAGS += -target arm64-apple-macos13
  endif
# fm_voice_neon.c superseded by hand-written assembly; skip this object
NEON_OBJ :=
else
NEON_OBJ :=
endif

OBJ := src/main.o src/wav_writer.o src/euclid.o src/osc.o src/kick.o src/snare.o src/hat.o src/melody.o src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/event_queue.o src/simple_voice.o
BIN := bin/euclid
TEST_BIN := bin/gen_sine
TONE_BIN := bin/gen_tones
NOISE_BIN := bin/gen_noise_delay
KICK_BIN := bin/gen_kick
SNARE_BIN := bin/gen_snare
HAT_BIN := bin/gen_hat
MELODY_BIN := bin/gen_melody
FM_BIN := bin/gen_fm
SEG_BIN := bin/segment
DRUMS_BIN := bin/segment_drums
DRUMS_MEL_BIN := bin/segment_drums_mel
DRUMS_BASS_BIN := bin/segment_drums_bass
BELLS_BIN := bin/gen_bells
CALM_BIN := bin/gen_calm
QUANTUM_BIN := bin/gen_quantum
PLUCK_BIN := bin/gen_pluck
BASS_BIN := bin/gen_bass
BASSQ_BIN := bin/gen_bass_quantum
BASSP_BIN := bin/gen_bass_plucky

SEG_OBJ := src/segment.o src/wav_writer.o

# Conditionally build GEN_OBJ based on USE_ASM
ifeq ($(USE_ASM),1)
# With assembly: include both ASM objects and C objects (C provides init/trigger, ASM provides process)  
GEN_OBJ := $(ASM_OBJ) src/generator.o src/osc.o src/kick.o src/snare.o src/hat.o src/melody.o src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/event_queue.o src/simple_voice.o src/delay.o
else
# Pure C build: include all C objects
GEN_OBJ := src/generator.o src/osc.o src/kick.o src/snare.o src/hat.o src/melody.o src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/event_queue.o src/simple_voice.o src/euclid.o src/limiter.o
endif
REALTIME_OBJ := src/main_realtime.o src/coreaudio.o src/video.o src/raster.o src/terrain.o src/particles.o src/shapes.o src/crt_fx.o

REALTIME_BIN := bin/realtime

all: $(SEG_BIN) $(REALTIME_BIN)

$(SEG_BIN): $(SEG_OBJ) $(GEN_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(REALTIME_BIN): $(REALTIME_OBJ) $(GEN_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Individual generator builds - conditional to avoid duplicate symbols
ifeq ($(USE_ASM),1)
$(TEST_BIN): src/gen_sine.c src/osc.o $(ASM_OBJ) src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(TONE_BIN): src/gen_tones.c src/osc.o $(ASM_OBJ) src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(NOISE_BIN): src/gen_noise_delay.c src/osc.o $(ASM_OBJ) src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(KICK_BIN): src/gen_kick.c src/kick.o $(ASM_OBJ) src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(SNARE_BIN): src/gen_snare.c src/snare.o $(ASM_OBJ) src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(HAT_BIN): src/gen_hat.c src/hat.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(MELODY_BIN): src/gen_melody.c src/melody.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(FM_BIN): src/gen_fm.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^
else
$(TEST_BIN): src/gen_sine.c src/osc.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(TONE_BIN): src/gen_tones.c src/osc.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(NOISE_BIN): src/gen_noise_delay.c src/osc.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(KICK_BIN): src/gen_kick.c src/kick.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(SNARE_BIN): src/gen_snare.c src/snare.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(HAT_BIN): src/gen_hat.c src/hat.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(MELODY_BIN): src/gen_melody.c src/melody.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(FM_BIN): src/gen_fm.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^
endif

$(DRUMS_BIN): src/segment.c $(SEG_OBJ) | bin
	$(CC) $(CFLAGS) -DDRUMS_ONLY -o $@ src/segment.c $(SEG_OBJ)

$(DRUMS_MEL_BIN): src/segment.c $(SEG_OBJ) | bin
	$(CC) $(CFLAGS) -DNO_FM -o $@ src/segment.c $(SEG_OBJ)

$(DRUMS_BASS_BIN): src/segment.c $(SEG_OBJ) | bin
	$(CC) $(CFLAGS) -DNO_MID_FM -o $@ src/segment.c $(SEG_OBJ)

# FM-related generator builds - conditional to avoid duplicate symbols
ifeq ($(USE_ASM),1)
$(BELLS_BIN): src/gen_bells.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(CALM_BIN): src/gen_calm.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(QUANTUM_BIN): src/gen_quantum.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(PLUCK_BIN): src/gen_pluck.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASS_BIN): src/gen_bass.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASSQ_BIN): src/gen_bass_quantum.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASSP_BIN): src/gen_bass_plucky.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o $(ASM_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^
else
$(BELLS_BIN): src/gen_bells.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(CALM_BIN): src/gen_calm.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(QUANTUM_BIN): src/gen_quantum.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(PLUCK_BIN): src/gen_pluck.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASS_BIN): src/gen_bass.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASSQ_BIN): src/gen_bass_quantum.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^

$(BASSP_BIN): src/gen_bass_plucky.c src/fm_voice.o $(NEON_OBJ) src/fm_presets.o src/wav_writer.o | bin
	$(CC) $(CFLAGS) -o $@ $^
endif

bin:
	@mkdir -p bin

# pattern rule for objects
src/%.o: src/%.c | src include
	$(CC) $(CFLAGS) -c $< -o $@

src include:
	@mkdir -p src include

$(ASM_DIR)/%.o: $(ASM_DIR)/%.s | $(ASM_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(ASM_DIR):
	@mkdir -p $(ASM_DIR)

.PHONY: clean
clean:
	rm -rf $(OBJ) $(BIN) $(ASM_DIR)/*.o 2>/dev/null || true

.PHONY: sine
sine: $(TEST_BIN)
	$(TEST_BIN)
	@echo "Generated sine.wav"

.PHONY: tones
tones: $(TONE_BIN)
	$(TONE_BIN)
	@echo "Generated saw.wav square.wav triangle.wav"

.PHONY: delay
delay: $(NOISE_BIN)
	$(NOISE_BIN)
	@echo "Generated delay.wav"

.PHONY: kick
kick: $(KICK_BIN)
	$(KICK_BIN)
	@echo "Generated kick.wav"

.PHONY: snare
snare: $(SNARE_BIN)
	$(SNARE_BIN)
	@echo "Generated snare.wav"

.PHONY: hat
hat: $(HAT_BIN)
	$(HAT_BIN)
	@echo "Generated hat.wav"

.PHONY: melody
melody: $(MELODY_BIN)
	$(MELODY_BIN)
	@echo "Generated melody.wav"

.PHONY: fm
fm: clean $(FM_BIN)
	$(FM_BIN)
	@echo "Generated fm.wav"

.PHONY: segment
segment: $(SEG_BIN)
	$(SEG_BIN)
	@echo "Generated segment.wav"

.PHONY: drums
drums: $(DRUMS_BIN)
	$(DRUMS_BIN)
	@echo "Generated segment_drums.wav"

.PHONY: drums_mel
drums_mel: $(DRUMS_MEL_BIN)
	$(DRUMS_MEL_BIN)
	@echo "Generated segment_drums_mel.wav"

.PHONY: drums_bass
drums_bass: $(DRUMS_BASS_BIN)
	$(DRUMS_BASS_BIN)
	@echo "Generated segment_drums_bass.wav"

.PHONY: bells
bells: $(BELLS_BIN)
	$(BELLS_BIN)
	@echo "Generated bells-c.wav"

.PHONY: calm
calm: $(CALM_BIN)
	$(CALM_BIN)
	@echo "Generated calm-c.wav"

.PHONY: quantum
quantum: $(QUANTUM_BIN)
	$(QUANTUM_BIN)
	@echo "Generated quantum-c.wav"

.PHONY: pluck
pluck: $(PLUCK_BIN)
	$(PLUCK_BIN)
	@echo "Generated pluck-c.wav"

.PHONY: bass
bass: $(BASS_BIN)
	$(BASS_BIN)
	@echo "Generated bass_only.wav"

.PHONY: realtime
realtime: $(REALTIME_BIN)

.PHONY: clang_check
clang_check:
	@clang -v >/dev/null 2>&1 && echo "clang OK" || echo "clang missing"

.PHONY: clean-asm
clean-asm:
	rm -f $(ASM_OBJ)

.PHONY: bass_quantum
bass_quantum: $(BASSQ_BIN)
	$(BASSQ_BIN)
	@echo "Generated bass_quantum.wav"

.PHONY: bass_plucky
bass_plucky: $(BASSP_BIN)
	$(BASSP_BIN)
	@echo "Generated bass_plucky.wav"

# Convenience target: build everything for arm64 on x86 hosts
.PHONY: cross
cross:
	$(MAKE) clean
	$(MAKE) all CROSS=1 -j$(shell sysctl -n hw.logicalcpu)