#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../include/visual_types.h"

#define TILE_SIZE 32
#define SCROLL_SPEED 2
#define TERRAIN_LENGTH 64

// Terrain tile types matching Python reference
typedef enum {
    TERRAIN_FLAT,
    TERRAIN_WALL, 
    TERRAIN_SLOPE_UP,
    TERRAIN_SLOPE_DOWN,
    TERRAIN_GAP
} terrain_type_t;

typedef struct {
    terrain_type_t type;
    int height;
} terrain_tile_t;

// Pre-generated terrain pattern and ASCII tile patterns
static terrain_tile_t terrain_pattern[TERRAIN_LENGTH];
static char tile_flat_pattern[TILE_SIZE * TILE_SIZE];
static char tile_slope_up_pattern[TILE_SIZE * TILE_SIZE];
static char tile_slope_down_pattern[TILE_SIZE * TILE_SIZE];
static color_t terrain_color;
static bool terrain_initialized = false;

// Forward declare ASCII drawing function
void draw_ascii_char(uint32_t *pixels, int x, int y, char c, uint32_t color, int alpha);

// Forward declare glitch functions
char get_glitched_terrain_char(char original_char, int x, int y, int frame);
char get_digital_noise_char(int x, int y, int frame);
bool should_apply_matrix_cascade(int x, int y, int frame);
char get_matrix_cascade_char(int x, int y, int frame);

// Generate deterministic terrain pattern (matching Python logic)
static void generate_terrain_pattern(uint32_t seed) {
    // Use XOR with magic number to match Python terrain_rng
    srand(seed ^ 0x7E44A1);
    
    int i = 0;
    while (i < TERRAIN_LENGTH) {
        int feature_choice = rand() % 5; // 0-4 for flat, wall, slope_up, slope_down, gap
        
        if (feature_choice == 0) { // flat
            int length = 2 + (rand() % 5); // 2-6 tiles
            for (int j = 0; j < length && i < TERRAIN_LENGTH; j++, i++) {
                terrain_pattern[i] = (terrain_tile_t){TERRAIN_FLAT, 2};
            }
        }
        else if (feature_choice == 1) { // wall
            int wall_height = (rand() % 2) ? 4 : 6; // 4 or 6
            int wall_width = 2 + (rand() % 3); // 2-4
            for (int j = 0; j < wall_width && i < TERRAIN_LENGTH; j++, i++) {
                terrain_pattern[i] = (terrain_tile_t){TERRAIN_WALL, wall_height};
            }
        }
        else if (feature_choice == 2) { // slope_up
            if (i < TERRAIN_LENGTH) {
                terrain_pattern[i++] = (terrain_tile_t){TERRAIN_SLOPE_UP, 2};
                // followed by elevated platform
                int length = 2 + (rand() % 3); // 2-4
                for (int j = 0; j < length && i < TERRAIN_LENGTH; j++, i++) {
                    terrain_pattern[i] = (terrain_tile_t){TERRAIN_FLAT, 3};
                }
            }
        }
        else if (feature_choice == 3) { // slope_down
            if (i < TERRAIN_LENGTH) {
                terrain_pattern[i++] = (terrain_tile_t){TERRAIN_SLOPE_DOWN, 3};
            }
        }
        else { // gap
            int length = 1 + (rand() % 2); // 1-2 tiles
            for (int j = 0; j < length && i < TERRAIN_LENGTH; j++, i++) {
                terrain_pattern[i] = (terrain_tile_t){TERRAIN_GAP, 0};
            }
        }
    }
}

// ASCII terrain character selection based on position
static char get_terrain_char(int x, int y) {
    // Use the same algorithm as Python but map to characters
    int h = ((x * 13 + y * 7) ^ (x >> 3)) & 0xFF;
    
    if (h < 40) {
        return '#';  // Solid rock
    } else if (h < 120) {
        return '=';  // Medium density
    } else {
        return '-';  // Light density
    }
}

// Build procedural ASCII tile pattern
static void build_ascii_tile_pattern(char *tile_pattern) {
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            tile_pattern[y * TILE_SIZE + x] = get_terrain_char(x, y);
        }
    }
}

// Build ASCII slope tile pattern (45 degree angle)
static void build_ascii_slope_pattern(char *tile_pattern, bool slope_up) {
    // Initialize to space (transparent)
    memset(tile_pattern, ' ', TILE_SIZE * TILE_SIZE);
    
    for (int y = 0; y < TILE_SIZE; y++) {
        for (int x = 0; x < TILE_SIZE; x++) {
            int threshold = slope_up ? x : (TILE_SIZE - x);
            
            if (y > threshold) {
                // Below slope - use terrain character
                tile_pattern[y * TILE_SIZE + x] = get_terrain_char(x, y);
            }
            // Above slope remains space (transparent)
        }
    }
}

// Initialize terrain system
void init_terrain(uint32_t seed, float base_hue) {
    if (terrain_initialized) return;
    
    generate_terrain_pattern(seed);
    
    // Create tile color (matching Python: base_hue + 0.3, saturation 1, value 0.8)
    hsv_t tile_hsv = {fmod(base_hue + 0.3f, 1.0f), 1.0f, 0.8f};
    terrain_color = hsv_to_rgb(tile_hsv);
    
    // Build ASCII tile patterns
    build_ascii_tile_pattern(tile_flat_pattern);
    build_ascii_slope_pattern(tile_slope_up_pattern, true);
    build_ascii_slope_pattern(tile_slope_down_pattern, false);
    
    terrain_initialized = true;
}

// Draw terrain to pixel buffer using ASCII characters
void draw_terrain(uint32_t *pixels, int frame) {
    if (!terrain_initialized) return;
    
    int char_width = 8;  // Character width in pixels
    int char_height = 12; // Character height in pixels
    
    int offset = (frame * SCROLL_SPEED) % TILE_SIZE;
    int tiles_per_screen = (VIS_WIDTH / TILE_SIZE) + 2;
    int scroll_tiles = (frame * SCROLL_SPEED) / TILE_SIZE;
    
    uint32_t color = color_to_pixel(terrain_color);
    
    for (int i = 0; i < tiles_per_screen; i++) {
        int terrain_idx = (scroll_tiles + i) % TERRAIN_LENGTH;
        terrain_tile_t terrain = terrain_pattern[terrain_idx];
        
        int x0 = i * TILE_SIZE - offset;
        
        if (terrain.type == TERRAIN_GAP) {
            continue; // skip drawing for gaps
        }
        
        // Draw tiles based on height
        for (int row = 0; row < terrain.height; row++) {
            int y0 = VIS_HEIGHT - (row + 1) * TILE_SIZE;
            
            // Choose which ASCII pattern to use
            char *pattern;
            if (terrain.type == TERRAIN_SLOPE_UP && row == terrain.height - 1) {
                pattern = tile_slope_up_pattern;
            } else if (terrain.type == TERRAIN_SLOPE_DOWN && row == terrain.height - 1) {
                pattern = tile_slope_down_pattern;
            } else {
                pattern = tile_flat_pattern;
            }
            
            // Draw ASCII characters from the pattern
            for (int ty = 0; ty < TILE_SIZE; ty += char_height) {
                for (int tx = 0; tx < TILE_SIZE; tx += char_width) {
                    int pattern_x = tx / char_width;
                    int pattern_y = ty / char_height;
                    
                    if (pattern_x < TILE_SIZE/char_width && pattern_y < TILE_SIZE/char_height) {
                        int pattern_idx = pattern_y * (TILE_SIZE/char_width) + pattern_x;
                        if (pattern_idx < TILE_SIZE * TILE_SIZE) {
                            char c = pattern[pattern_idx];
                            
                            if (c != ' ') { // Skip transparent characters
                                int screen_x = x0 + tx;
                                int screen_y = y0 + ty;
                                
                                if (screen_x >= 0 && screen_x < VIS_WIDTH - char_width && 
                                    screen_y >= 0 && screen_y < VIS_HEIGHT - char_height) {
                                    
                                    // Apply glitch effects to the character
                                    char glitched_char = c;
                                    
                                    // Check for matrix cascade first (overrides other effects)
                                    if (should_apply_matrix_cascade(screen_x, screen_y, frame)) {
                                        glitched_char = get_matrix_cascade_char(screen_x, screen_y, frame);
                                    } else {
                                        // Apply terrain glitch
                                        glitched_char = get_glitched_terrain_char(c, screen_x, screen_y, frame);
                                    }
                                    
                                    draw_ascii_char(pixels, screen_x, screen_y, glitched_char, color, 255);
                                }
                            } else {
                                // Even on transparent areas, occasionally show digital noise
                                int screen_x = x0 + tx;
                                int screen_y = y0 + ty;
                                
                                if (screen_x >= 0 && screen_x < VIS_WIDTH - char_width && 
                                    screen_y >= 0 && screen_y < VIS_HEIGHT - char_height) {
                                    
                                    char noise_char = get_digital_noise_char(screen_x, screen_y, frame);
                                    if (noise_char != ' ') {
                                        // Use dimmer color for noise
                                        draw_ascii_char(pixels, screen_x, screen_y, noise_char, color, 128);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
