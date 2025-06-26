#ifndef PARTICLES_H
#define PARTICLES_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PARTICLES 256

typedef struct {
    float x,y;
    float vx,vy;
    int life;
    int max_life;
    uint32_t color;
    uint8_t glyph; /* index into glyph set */
} particle_t;

void particles_init(void);
void particles_spawn_burst(float x,float y,int count,uint32_t color);
void particles_update_and_draw(uint32_t *fb,int w,int h);

#endif /* PARTICLES_H */ 