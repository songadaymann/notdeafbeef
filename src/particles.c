#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "../include/visual_types.h"

#define MAX_PARTICLES 256
#define PARTICLE_SPAWN_COUNT 20
#define GRAVITY 0.1f

// SAW_STEPS when particle explosions occur (matching Python)
static const int SAW_STEPS[] = {0, 8, 16, 24};
static const int SAW_STEPS_COUNT = 4;

// Character set for particles (matching Python glyph_set)
static const char PARTICLE_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*+-=?";
static const int CHAR_COUNT = sizeof(PARTICLE_CHARS) - 1;

typedef struct {
    float x, y;           // Position
    float vx, vy;         // Velocity
    int life;             // Life remaining
    int max_life;         // Initial life for alpha calculation
    char character;       // Character to display
    color_t color;        // Particle color
    bool active;          // Is this particle slot active
} particle_t;

static particle_t particles[MAX_PARTICLES];
static bool particles_initialized = false;
static int last_step = -1;  // Track last step for spawn prevention

// Forward declare ASCII rendering function
void draw_ascii_char(uint32_t *pixels, int x, int y, char c, uint32_t color, int alpha);

void init_particles(void) {
    if (particles_initialized) return;
    
    // Initialize all particles as inactive
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = false;
    }
    
    particles_initialized = true;
}

// Check if step is a saw step (particle explosion trigger)
static bool is_saw_step(int step) {
    int step32 = step % 32;  // 2-bar cycle (32 sixteenth-notes)
    for (int i = 0; i < SAW_STEPS_COUNT; i++) {
        if (step32 == SAW_STEPS[i]) {
            return true;
        }
    }
    return false;
}

// Spawn particle explosion at given position
static void spawn_explosion(float cx, float cy, float base_hue) {
    for (int i = 0; i < PARTICLE_SPAWN_COUNT; i++) {
        // Find free particle slot
        int slot = -1;
        for (int j = 0; j < MAX_PARTICLES; j++) {
            if (!particles[j].active) {
                slot = j;
                break;
            }
        }
        
        if (slot == -1) break; // No free slots
        
        particle_t *p = &particles[slot];
        
        // Circular explosion pattern
        float angle = 2.0f * M_PI * i / PARTICLE_SPAWN_COUNT;
        float speed = 2.0f + (rand() / (float)RAND_MAX) * 2.0f; // 2-4 speed
        
        p->x = cx;
        p->y = cy;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        
        // Random character
        p->character = PARTICLE_CHARS[rand() % CHAR_COUNT];
        
        // Color with slight hue variation
        float hue_offset = ((rand() / (float)RAND_MAX) - 0.5f) * 0.2f; // -0.1 to +0.1
        hsv_t particle_hsv = {fmodf(base_hue + hue_offset, 1.0f), 1.0f, 1.0f};
        p->color = hsv_to_rgb(particle_hsv);
        
        // Random life span
        int life_options[] = {30, 60, 90};
        p->life = life_options[rand() % 3];
        p->max_life = p->life;
        
        p->active = true;
    }
}

void update_particles(float elapsed_ms, float step_sec, float base_hue) {
    if (!particles_initialized) return;
    
    // Calculate current step for explosion timing
    int current_step = (int)(elapsed_ms / 1000.0f / step_sec) % 32;
    
    // Spawn explosion if we hit a saw step and it's different from last step
    if (is_saw_step(current_step) && current_step != last_step) {
        last_step = current_step;
        
        // Random explosion position (matching Python ranges)
        float cx = VIS_WIDTH * 0.3f + (rand() / (float)RAND_MAX) * (VIS_WIDTH * 0.4f);  // 0.3 to 0.7
        float cy = VIS_HEIGHT * 0.2f + (rand() / (float)RAND_MAX) * (VIS_HEIGHT * 0.3f); // 0.2 to 0.5
        
        spawn_explosion(cx, cy, base_hue);
    }
    
    // Update all active particles
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_t *p = &particles[i];
        
        if (!p->active) continue;
        
        // Update physics
        p->x += p->vx;
        p->y += p->vy;
        p->vy += GRAVITY; // Gravity
        
        // Update life
        p->life--;
        
        // Deactivate if life expired
        if (p->life <= 0) {
            p->active = false;
        }
    }
}

void draw_particles(uint32_t *pixels) {
    if (!particles_initialized) return;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_t *p = &particles[i];
        
        if (!p->active || p->life <= 0) continue;
        
        // Calculate alpha based on remaining life
        int alpha = (255 * p->life) / p->max_life;
        
        // Convert color to pixel format
        uint32_t color = color_to_pixel(p->color);
        
        // Draw character using improved ASCII renderer
        draw_ascii_char(pixels, (int)p->x, (int)p->y, p->character, color, alpha);
    }
}

void reset_particle_step_tracking(void) {
    last_step = -1;
}
