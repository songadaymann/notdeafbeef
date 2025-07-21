#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../include/visual_types.h"

#define MAX_BASS_HITS 16
#define STEPS_PER_BEAT 4

// Bass hit shape types (matching Python reference)
typedef enum {
    BASS_TRIANGLE,
    BASS_DIAMOND,
    BASS_HEXAGON,
    BASS_STAR,
    BASS_SQUARE
} bass_shape_type_t;

typedef struct {
    bass_shape_type_t shape_type;
    color_t color;
    int alpha;               // Current alpha (0-255)
    float scale;             // Current scale (0.0-1.0+)
    float max_size;          // Maximum size in pixels
    float rotation;          // Current rotation angle
    float rot_speed;         // Rotation speed
    bool active;             // Is this bass hit active
} bass_hit_t;

static bass_hit_t bass_hits[MAX_BASS_HITS];
static bool bass_hits_initialized = false;
static int last_bass_step = -1;

// Forward declare glitch and ASCII functions
char get_glitched_shape_char(char original_char, int x, int y, int frame);
void draw_ascii_char(uint32_t *pixels, int x, int y, char c, uint32_t color, int alpha);

// Initialize bass hit system
void init_bass_hits(void) {
    if (bass_hits_initialized) return;
    
    // Initialize all bass hits as inactive
    for (int i = 0; i < MAX_BASS_HITS; i++) {
        bass_hits[i].active = false;
    }
    
    bass_hits_initialized = true;
}

// Create a new bass hit
static void spawn_bass_hit(float base_hue, uint32_t seed) {
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_BASS_HITS; i++) {
        if (!bass_hits[i].active) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) return; // No free slots
    
    bass_hit_t *hit = &bass_hits[slot];
    
    // Choose random shape type
    srand(seed + slot);
    int shape_types[] = {BASS_TRIANGLE, BASS_DIAMOND, BASS_HEXAGON, BASS_STAR, BASS_SQUARE};
    hit->shape_type = shape_types[rand() % 5];
    
    // Color with hue shift
    float hue_shift = ((rand() / (float)RAND_MAX) - 0.5f) * 0.4f; // -0.2 to +0.2
    hsv_t hit_hsv = {fmod(base_hue + hue_shift, 1.0f), 1.0f, 1.0f};
    hit->color = hsv_to_rgb(hit_hsv);
    
    // Initial properties
    hit->alpha = 255;
    hit->scale = 0.1f;                                    // Start small
    hit->max_size = (VIS_WIDTH < VIS_HEIGHT ? VIS_WIDTH : VIS_HEIGHT) * 0.6f;  // 60% of smaller dimension
    hit->rotation = 0.0f;
    hit->rot_speed = ((rand() / (float)RAND_MAX) - 0.5f) * 0.1f;  // -0.05 to +0.05
    hit->active = true;
}

// Draw ASCII triangle
static void draw_ascii_triangle(uint32_t *pixels, int cx, int cy, int size, float rotation, uint32_t color, int alpha, int frame) {
    if (size < 8) return; // Too small to draw meaningfully
    
    char triangle_chars[] = {'^', 'A', '/', '\\', '-'};
    int char_count = sizeof(triangle_chars) / sizeof(triangle_chars[0]);
    
    // Draw triangle outline using characters
    for (int i = 0; i < 3; i++) {
        float angle = rotation + i * 2.0f * M_PI / 3.0f;
        int x = cx + (int)(cosf(angle) * size * 0.8f);
        int y = cy + (int)(sinf(angle) * size * 0.8f);
        
        char base_char = triangle_chars[i % char_count];
        char glitched_char = get_glitched_shape_char(base_char, x, y, frame);
        draw_ascii_char(pixels, x, y, glitched_char, color, alpha);
        
        // Draw connecting lines
        float next_angle = rotation + ((i + 1) % 3) * 2.0f * M_PI / 3.0f;
        int next_x = cx + (int)(cosf(next_angle) * size * 0.8f);
        int next_y = cy + (int)(sinf(next_angle) * size * 0.8f);
        
        // Simple line drawing with characters
        int steps = abs(next_x - x) + abs(next_y - y);
        steps = steps / 12; // Character spacing
        for (int step = 1; step < steps; step++) {
            float t = (float)step / steps;
            int line_x = x + (int)(t * (next_x - x));
            int line_y = y + (int)(t * (next_y - y));
            char line_char = get_glitched_shape_char('-', line_x, line_y, frame);
            draw_ascii_char(pixels, line_x, line_y, line_char, color, alpha - 50);
        }
    }
}

// Draw ASCII diamond
static void draw_ascii_diamond(uint32_t *pixels, int cx, int cy, int size, float rotation, uint32_t color, int alpha, int frame) {
    if (size < 8) return;
    
    char diamond_chars[] = {'<', '>', '^', 'v', '*'};
    int char_count = sizeof(diamond_chars) / sizeof(diamond_chars[0]);
    
    // Draw diamond points
    int points[4][2] = {
        {cx, cy - size},     // Top
        {cx + size, cy},     // Right  
        {cx, cy + size},     // Bottom
        {cx - size, cy}      // Left
    };
    
    // Rotate points around center
    for (int i = 0; i < 4; i++) {
        float dx = points[i][0] - cx;
        float dy = points[i][1] - cy;
        float rotated_x = dx * cosf(rotation) - dy * sinf(rotation);
        float rotated_y = dx * sinf(rotation) + dy * cosf(rotation);
        points[i][0] = cx + (int)rotated_x;
        points[i][1] = cy + (int)rotated_y;
        
        char base_char = diamond_chars[i % char_count];
        char glitched_char = get_glitched_shape_char(base_char, points[i][0], points[i][1], frame);
        draw_ascii_char(pixels, points[i][0], points[i][1], glitched_char, color, alpha);
    }
    
    // Connect the points
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        int steps = (abs(points[next][0] - points[i][0]) + abs(points[next][1] - points[i][1])) / 12;
        for (int step = 1; step < steps; step++) {
            float t = (float)step / steps;
            int line_x = points[i][0] + (int)(t * (points[next][0] - points[i][0]));
            int line_y = points[i][1] + (int)(t * (points[next][1] - points[i][1]));
            char line_char = get_glitched_shape_char('=', line_x, line_y, frame);
            draw_ascii_char(pixels, line_x, line_y, line_char, color, alpha - 50);
        }
    }
}

// Draw ASCII hexagon
static void draw_ascii_hexagon(uint32_t *pixels, int cx, int cy, int size, float rotation, uint32_t color, int alpha, int frame) {
    if (size < 8) return;
    
    char hex_chars[] = {'O', '0', '#', '*', '+', 'X'};
    int char_count = sizeof(hex_chars) / sizeof(hex_chars[0]);
    
    // Draw hexagon vertices
    for (int i = 0; i < 6; i++) {
        float angle = rotation + i * M_PI / 3.0f;
        int x = cx + (int)(cosf(angle) * size * 0.7f);
        int y = cy + (int)(sinf(angle) * size * 0.7f);
        
        char base_char = hex_chars[i % char_count];
        char glitched_char = get_glitched_shape_char(base_char, x, y, frame);
        draw_ascii_char(pixels, x, y, glitched_char, color, alpha);
    }
}

// Draw ASCII star
static void draw_ascii_star(uint32_t *pixels, int cx, int cy, int size, float rotation, uint32_t color, int alpha, int frame) {
    if (size < 8) return;
    
    char star_chars[] = {'*', '+', 'x', 'X', '^', 'v', '<', '>'};
    int char_count = sizeof(star_chars) / sizeof(star_chars[0]);
    
    // Draw 5-pointed star (10 points alternating inner/outer)
    for (int i = 0; i < 10; i++) {
        float angle = rotation + i * M_PI / 5.0f;
        float radius = (i % 2 == 0) ? size * 0.8f : size * 0.4f; // Alternate between outer and inner
        int x = cx + (int)(cosf(angle) * radius);
        int y = cy + (int)(sinf(angle) * radius);
        
        char base_char = star_chars[i % char_count];
        char glitched_char = get_glitched_shape_char(base_char, x, y, frame);
        draw_ascii_char(pixels, x, y, glitched_char, color, alpha);
    }
}

// Draw ASCII square
static void draw_ascii_square(uint32_t *pixels, int cx, int cy, int size, float rotation, uint32_t color, int alpha, int frame) {
    if (size < 8) return;
    
    char square_chars[] = {'#', '=', '+', 'H', 'M', 'W'};
    int char_count = sizeof(square_chars) / sizeof(square_chars[0]);
    
    // Draw square corners
    int half = size;
    int corners[4][2] = {
        {cx - half, cy - half},  // Top-left
        {cx + half, cy - half},  // Top-right
        {cx + half, cy + half},  // Bottom-right
        {cx - half, cy + half}   // Bottom-left
    };
    
    // Rotate corners around center
    for (int i = 0; i < 4; i++) {
        float dx = corners[i][0] - cx;
        float dy = corners[i][1] - cy;
        float rotated_x = dx * cosf(rotation) - dy * sinf(rotation);
        float rotated_y = dx * sinf(rotation) + dy * cosf(rotation);
        corners[i][0] = cx + (int)rotated_x;
        corners[i][1] = cy + (int)rotated_y;
        
        char base_char = square_chars[i % char_count];
        char glitched_char = get_glitched_shape_char(base_char, corners[i][0], corners[i][1], frame);
        draw_ascii_char(pixels, corners[i][0], corners[i][1], glitched_char, color, alpha);
    }
    
    // Draw square edges
    for (int i = 0; i < 4; i++) {
        int next = (i + 1) % 4;
        int steps = (abs(corners[next][0] - corners[i][0]) + abs(corners[next][1] - corners[i][1])) / 12;
        for (int step = 1; step < steps; step++) {
            float t = (float)step / steps;
            int line_x = corners[i][0] + (int)(t * (corners[next][0] - corners[i][0]));
            int line_y = corners[i][1] + (int)(t * (corners[next][1] - corners[i][1]));
            char line_char = get_glitched_shape_char('-', line_x, line_y, frame);
            draw_ascii_char(pixels, line_x, line_y, line_char, color, alpha - 50);
        }
    }
}

// Update bass hits (handles spawning and animation)
void update_bass_hits(float elapsed_ms, float step_sec, float base_hue, uint32_t seed) {
    if (!bass_hits_initialized) return;
    
    // Calculate current step for bass hit timing
    int current_step = (int)(elapsed_ms / 1000.0f / step_sec) % 32;
    
    // Spawn bass hit every 8 beats (2 bars) - matching Python: current_step % (STEPS_PER_BEAT*8) == 0
    if (current_step % (STEPS_PER_BEAT * 8) == 0 && current_step != last_bass_step) {
        last_bass_step = current_step;
        spawn_bass_hit(base_hue, seed + current_step);
    }
    
    // Update all active bass hits
    for (int i = 0; i < MAX_BASS_HITS; i++) {
        bass_hit_t *hit = &bass_hits[i];
        
        if (!hit->active) continue;
        
        // Grow rapidly then fade (matching Python behavior)
        if (hit->scale < 1.0f) {
            hit->scale += 0.15f; // Fast growth
        }
        hit->alpha = hit->alpha > 8 ? hit->alpha - 8 : 0; // Fade out
        hit->rotation += hit->rot_speed;
        
        // Deactivate if fully faded
        if (hit->alpha <= 0) {
            hit->active = false;
        }
    }
}

// Draw all active bass hits
void draw_bass_hits(uint32_t *pixels, int frame) {
    if (!bass_hits_initialized) return;
    
    int cx = VIS_WIDTH / 2;
    int cy = VIS_HEIGHT / 2;
    
    for (int i = 0; i < MAX_BASS_HITS; i++) {
        bass_hit_t *hit = &bass_hits[i];
        
        if (!hit->active || hit->alpha <= 0) continue;
        
        int size = (int)(hit->max_size * fminf(hit->scale, 1.0f));
        uint32_t color = color_to_pixel(hit->color);
        
        // Draw the appropriate shape
        switch (hit->shape_type) {
            case BASS_TRIANGLE:
                draw_ascii_triangle(pixels, cx, cy, size, hit->rotation, color, hit->alpha, frame);
                break;
            case BASS_DIAMOND:
                draw_ascii_diamond(pixels, cx, cy, size, hit->rotation, color, hit->alpha, frame);
                break;
            case BASS_HEXAGON:
                draw_ascii_hexagon(pixels, cx, cy, size, hit->rotation, color, hit->alpha, frame);
                break;
            case BASS_STAR:
                draw_ascii_star(pixels, cx, cy, size, hit->rotation, color, hit->alpha, frame);
                break;
            case BASS_SQUARE:
                draw_ascii_square(pixels, cx, cy, size, hit->rotation, color, hit->alpha, frame);
                break;
        }
    }
}

// Reset bass hit step tracking
void reset_bass_hit_step_tracking(void) {
    last_bass_step = -1;
}
