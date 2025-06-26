# NotDeafbeef - Root Build System
# Orchestrates builds for both C and Assembly implementations

# Default target builds the stable configuration
all: c-build

# Build C implementation (stable)
c-build:
	$(MAKE) -C src/c

# Generate test audio files  
test-audio:
	python tools/generate_test_wavs.py

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

.PHONY: all c-build test-audio test clean demo verify
