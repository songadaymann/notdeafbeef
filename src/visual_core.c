#include <math.h>
#include <stdlib.h>
#include "../include/visual_types.h"

// Convert HSV to RGB (matching Python colorsys behavior)
color_t hsv_to_rgb(hsv_t hsv) {
    color_t rgb = {0};
    
    float h = hsv.h;
    float s = hsv.s;
    float v = hsv.v;
    
    // Ensure hue is in [0, 1) range
    h = fmod(h, 1.0f);
    if (h < 0) h += 1.0f;
    
    // Clamp saturation and value
    s = fmaxf(0.0f, fminf(1.0f, s));
    v = fmaxf(0.0f, fminf(1.0f, v));
    
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    
    switch (i % 6) {
        case 0: rgb.r = v * 255; rgb.g = t * 255; rgb.b = p * 255; break;
        case 1: rgb.r = q * 255; rgb.g = v * 255; rgb.b = p * 255; break;
        case 2: rgb.r = p * 255; rgb.g = v * 255; rgb.b = t * 255; break;
        case 3: rgb.r = p * 255; rgb.g = q * 255; rgb.b = v * 255; break;
        case 4: rgb.r = t * 255; rgb.g = p * 255; rgb.b = v * 255; break;
        case 5: rgb.r = v * 255; rgb.g = p * 255; rgb.b = q * 255; break;
    }
    
    rgb.a = 255;
    return rgb;
}

// Convert RGB to 32-bit ARGB pixel (SDL2 format)
uint32_t color_to_pixel(color_t color) {
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

// Determine visual mode based on BPM (matching Python logic)
visual_mode_t get_visual_mode(int bpm) {
    if (bpm < 70) return VIS_MODE_THICK;
    if (bpm < 100) return VIS_MODE_RINGS;
    if (bpm < 130) return VIS_MODE_POLY;
    return VIS_MODE_LISSA;
}

// Initialize degradation effects based on seed
void init_degradation_effects(degradation_t *effects, uint32_t seed) {
    // Use XOR with magic number to match Python degrade_rng
    srand(seed ^ 0xDE5A7);
    
    // Randomize effect levels matching Python uniform distributions
    effects->persistence = 0.3f + (rand() / (float)RAND_MAX) * 0.6f;      // 0.3-0.9
    effects->scanline_alpha = rand() % 201;                                // 0-200
    effects->chroma_shift = rand() % 6;                                    // 0-5
    effects->noise_pixels = rand() % 301;                                  // 0-300
    effects->jitter_amount = (rand() / (float)RAND_MAX) * 3.0f;          // 0-3
    effects->frame_drop_chance = (rand() / (float)RAND_MAX) * 0.1f;       // 0-0.1
    effects->color_bleed = (rand() / (float)RAND_MAX) * 0.3f;             // 0-0.3
}

// Initialize centerpiece based on seed and BPM
void init_centerpiece(centerpiece_t *centerpiece, uint32_t seed, int bpm) {
    srand(seed);
    
    centerpiece->mode = get_visual_mode(bpm);
    centerpiece->bpm = bpm;
    centerpiece->orbit_radius = 80.0f + (rand() / (float)RAND_MAX) * 40.0f; // 80-120
    centerpiece->base_hue = rand() / (float)RAND_MAX;                       // 0-1
    centerpiece->orbit_speed = 0.2f;                                       // Match Python t*0.2
}
