#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "../include/visual_types.h"

// Character sets for different glitch types
static const char TERRAIN_GLITCH_CHARS[] = "#=-%*+~^|\\/<>[]{}()";
static const char SHAPE_GLITCH_CHARS[] = "@#*+=-|\\/<>^~`'\".:;!?";
static const char DIGITAL_NOISE_CHARS[] = "01234567890123456789abcdefABCDEF!@#$%^&*";
static const char MATRIX_CHARS[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*+-=";

// Glitch parameters
typedef struct {
    float terrain_glitch_rate;    // How often terrain chars change (0-1)
    float shape_glitch_rate;      // How often shape chars change (0-1)
    float digital_noise_rate;     // Random digital noise overlay (0-1)
    float glitch_intensity;       // Overall glitch intensity (0-1)
    uint32_t glitch_seed;         // Seed for glitch randomness
} glitch_config_t;

static glitch_config_t glitch_config;
static bool glitch_initialized = false;

// Initialize glitch system
void init_glitch_system(uint32_t seed, float intensity) {
    if (glitch_initialized) return;
    
    // Use different seed for glitch to avoid correlation with other systems
    srand(seed ^ 0xDECAF);
    
    glitch_config.glitch_intensity = intensity;
    glitch_config.terrain_glitch_rate = 0.1f + intensity * 0.4f;  // 0.1 to 0.5
    glitch_config.shape_glitch_rate = 0.05f + intensity * 0.3f;   // 0.05 to 0.35
    glitch_config.digital_noise_rate = intensity * 0.1f;          // 0 to 0.1
    glitch_config.glitch_seed = seed;
    
    glitch_initialized = true;
}

// Get time-based random value for position
static uint32_t get_glitch_random(int x, int y, int frame) {
    // Create pseudo-random value based on position and time
    uint32_t hash = (uint32_t)x * 73 + (uint32_t)y * 37 + (uint32_t)frame * 17;
    hash ^= glitch_config.glitch_seed;
    
    // Simple LCG for pseudo-random
    hash = hash * 1664525 + 1013904223;
    return hash;
}

// Get glitched terrain character
char get_glitched_terrain_char(char original_char, int x, int y, int frame) {
    if (!glitch_initialized) return original_char;
    
    uint32_t rand_val = get_glitch_random(x, y, frame);
    float rand_float = (rand_val % 10000) / 10000.0f;
    
    // Apply glitch rate
    if (rand_float < glitch_config.terrain_glitch_rate) {
        int char_count = sizeof(TERRAIN_GLITCH_CHARS) - 1;
        int char_idx = (rand_val >> 8) % char_count;
        return TERRAIN_GLITCH_CHARS[char_idx];
    }
    
    return original_char;
}

// Get glitched shape character  
char get_glitched_shape_char(char original_char, int x, int y, int frame) {
    if (!glitch_initialized) return original_char;
    
    uint32_t rand_val = get_glitch_random(x, y, frame);
    float rand_float = (rand_val % 10000) / 10000.0f;
    
    // Apply glitch rate
    if (rand_float < glitch_config.shape_glitch_rate) {
        int char_count = sizeof(SHAPE_GLITCH_CHARS) - 1;
        int char_idx = (rand_val >> 8) % char_count;
        return SHAPE_GLITCH_CHARS[char_idx];
    }
    
    return original_char;
}

// Get completely random glitch character for digital noise
char get_digital_noise_char(int x, int y, int frame) {
    if (!glitch_initialized) return ' ';
    
    uint32_t rand_val = get_glitch_random(x, y, frame * 3); // Faster change rate
    float rand_float = (rand_val % 10000) / 10000.0f;
    
    // Apply noise rate
    if (rand_float < glitch_config.digital_noise_rate) {
        int char_count = sizeof(DIGITAL_NOISE_CHARS) - 1;
        int char_idx = (rand_val >> 8) % char_count;
        return DIGITAL_NOISE_CHARS[char_idx];
    }
    
    return ' '; // Transparent
}

// Check if we should apply matrix-style cascading effect
bool should_apply_matrix_cascade(int x, int y, int frame) {
    if (!glitch_initialized) return false;
    
    // Create vertical cascading lines
    uint32_t rand_val = get_glitch_random(x / 8, 0, frame / 10); // Column-based
    float cascade_chance = 0.02f * glitch_config.glitch_intensity;
    
    return ((rand_val % 10000) / 10000.0f) < cascade_chance;
}

// Get matrix cascade character
char get_matrix_cascade_char(int x, int y, int frame) {
    uint32_t rand_val = get_glitch_random(x, y, frame);
    int char_count = sizeof(MATRIX_CHARS) - 1;
    int char_idx = (rand_val >> 8) % char_count;
    return MATRIX_CHARS[char_idx];
}

// Update glitch intensity (can be driven by audio)
void update_glitch_intensity(float new_intensity) {
    if (!glitch_initialized) return;
    
    glitch_config.glitch_intensity = new_intensity;
    glitch_config.terrain_glitch_rate = 0.1f + new_intensity * 0.4f;
    glitch_config.shape_glitch_rate = 0.05f + new_intensity * 0.3f;
    glitch_config.digital_noise_rate = new_intensity * 0.1f;
}

// Get current glitch intensity
float get_glitch_intensity(void) {
    return glitch_initialized ? glitch_config.glitch_intensity : 0.0f;
}
