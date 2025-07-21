#ifndef VISUAL_TYPES_H
#define VISUAL_TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Visual constants matching Python reference
#define VIS_WIDTH 512
#define VIS_HEIGHT 512
#define VIS_FPS 30

// Visual mode enumeration based on BPM ranges
typedef enum {
    VIS_MODE_THICK,  // BPM < 70: Simple thick circle outline
    VIS_MODE_RINGS,  // 70-100: Multi-layered concentric rings  
    VIS_MODE_POLY,   // 100-130: Rotating polygon
    VIS_MODE_LISSA   // 130+: Lissajous figure-8 pattern
} visual_mode_t;

// RGB color structure (0-255 range)
typedef struct {
    uint8_t r, g, b, a;
} color_t;

// HSV color for easier manipulation (0.0-1.0 range)
typedef struct {
    float h, s, v;
} hsv_t;

// Point structure for geometry
typedef struct {
    int x, y;
} point_t;

// Floating point for precise calculations
typedef struct {
    float x, y;
} pointf_t;

// Visual state for the orbiting centerpiece
typedef struct {
    visual_mode_t mode;
    float orbit_radius;
    float base_hue;
    float orbit_speed;
    int bpm;
} centerpiece_t;

// Degradation effects parameters (seed-randomized)
typedef struct {
    float persistence;      // Ghost trails: 0.3-0.9
    int scanline_alpha;     // Scanline intensity: 0-200
    int chroma_shift;       // RGB channel shift: 0-5 pixels
    int noise_pixels;       // Random noise: 0-300 pixels
    float jitter_amount;    // Screen shake: 0-3 pixels
    float frame_drop_chance; // Repeat frames: 0-0.1
    float color_bleed;      // Horizontal blur: 0-0.3
} degradation_t;

// Main visual context
typedef struct {
    uint32_t *pixels;       // ARGB pixel buffer
    centerpiece_t centerpiece;
    degradation_t effects;
    uint32_t seed;
    float time;             // Current time in seconds
    int frame;              // Current frame number
    float step_sec;         // Duration of one step in seconds
    int bpm;                // Beats per minute
} visual_context_t;

// Function to convert HSV to RGB
color_t hsv_to_rgb(hsv_t hsv);

// Function to convert RGB to 32-bit ARGB pixel
uint32_t color_to_pixel(color_t color);

// Determine visual mode based on BPM
visual_mode_t get_visual_mode(int bpm);

// Drawing functions
void draw_centerpiece(uint32_t *pixels, centerpiece_t *centerpiece, float time, float level, int frame);

// Terrain system functions
void init_terrain(uint32_t seed, float base_hue);
void draw_terrain(uint32_t *pixels, int frame);

// Particle system functions
void init_particles(void);
void update_particles(float elapsed_ms, float step_sec, float base_hue);
void draw_particles(uint32_t *pixels);
void reset_particle_step_tracking(void);

// ASCII rendering functions
void draw_ascii_char(uint32_t *pixels, int x, int y, char c, uint32_t color, int alpha);
void draw_ascii_string(uint32_t *pixels, int x, int y, const char *str, uint32_t color, int alpha);
void draw_ascii_circle(uint32_t *pixels, int cx, int cy, int radius, char fill_char, uint32_t color, int alpha);
void draw_ascii_rect(uint32_t *pixels, int x, int y, int width, int height, char fill_char, uint32_t color, int alpha);
int get_char_width(void);
int get_char_height(void);

// Glitch system functions
void init_glitch_system(uint32_t seed, float intensity);
char get_glitched_terrain_char(char original_char, int x, int y, int frame);
char get_glitched_shape_char(char original_char, int x, int y, int frame);
char get_digital_noise_char(int x, int y, int frame);
bool should_apply_matrix_cascade(int x, int y, int frame);
char get_matrix_cascade_char(int x, int y, int frame);
void update_glitch_intensity(float new_intensity);
float get_glitch_intensity(void);

// Bass hit system functions
void init_bass_hits(void);
void update_bass_hits(float elapsed_ms, float step_sec, float base_hue, uint32_t seed);
void draw_bass_hits(uint32_t *pixels, int frame);
void reset_bass_hit_step_tracking(void);

// WAV reader functions
bool load_wav_file(const char *filename);
float get_audio_rms_for_frame(int frame);
float get_audio_time_for_frame(int frame);
float get_audio_duration(void);
float get_audio_bpm(void);
bool is_audio_finished(int frame);
float get_max_rms(void);
void cleanup_audio_data(void);
void print_audio_info(void);
void start_audio_playback(void);
void stop_audio_playback(void);
float get_playback_position(void);

#endif /* VISUAL_TYPES_H */
