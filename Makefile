# NotDeafbeef - Root Build System
# Orchestrates builds for both C and Assembly implementations

# Default target builds the stable configuration
all: c-build

# Build C implementation (stable)
c-build:
	$(MAKE) -C src/c

# Build visual system (isolated from audio)
vis-build:
	gcc -o bin/vis_main src/vis_main.c src/visual_core.c src/drawing.c src/terrain.c src/particles.c src/ascii_renderer.c src/glitch_system.c src/bass_hits.c src/wav_reader.c -Iinclude $(shell pkg-config --cflags --libs sdl2) -lm

# Build audio system only (for protection verification)
audio:
	$(MAKE) -C src/c segment USE_ASM=1 VOICE_ASM="GENERATOR_ASM KICK_ASM SNARE_ASM HAT_ASM MELODY_ASM LIMITER_ASM"

# Generate test audio files  
test-audio:
	python3 tools/generate_test_wavs.py

# NEW: Generate comprehensive WAV tests for all sounds in both C and ASM
test-comprehensive:
	python3 tools/generate_comprehensive_tests.py

# NEW: Compare C vs ASM WAV files
compare:
	python3 tools/compare_c_vs_asm.py

# NEW: Play specific sound for audition (usage: make play SOUND=kick)
play:
ifndef SOUND
	@echo "Usage: make play SOUND=<sound_name>"
	@echo "Example: make play SOUND=kick"
else
	python3 tools/compare_c_vs_asm.py --play $(SOUND)
endif

# Run test suite
test:
	pytest tests/

# Clean all build artifacts
clean:
	$(MAKE) -C src/c clean
	rm -rf output/
	find . -name "*.o" -delete
	find . -name "*.dSYM" -delete

# Generate a demo audio segment
demo:
	$(MAKE) -C src/c segment
	@echo "Generated demo audio: src/c/seed_0xcafebabe.wav"

# Quick verification that everything works
verify: c-build test-audio
	@echo "✅ NotDeafbeef verification complete!"

# NEW: Full verification including comprehensive tests
verify-full: c-build test-comprehensive compare
	@echo "✅ NotDeafbeef full verification complete!"
	@echo "Check the comparison output above for any issues."

.PHONY: all c-build vis-build audio test-audio test-comprehensive compare play test clean demo verify verify-full
