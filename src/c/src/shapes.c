#include "shapes.h"
#include "raster.h"
#include <math.h>
#include <stdlib.h>

static bass_shape_t g_shapes[MAX_SHAPES];
static int g_count = 0;

void shapes_init(void) { g_count = 0; }

void shapes_spawn(shape_type_t type, uint32_t color)
{
    if(g_count >= MAX_SHAPES) return;
    bass_shape_t *s = &g_shapes[g_count++];
    s->type = type;
    s->scale = 0.1f;
    s->rotation = 0.0f;
    s->rot_speed = -0.05f + (rand()%100)/1000.0f; /* -0.05 to +0.05 */
    s->alpha = 255;
    s->color = color;
}

static void rotate_point(float *x, float *y, float cx, float cy, float angle)
{
    float dx = *x - cx;
    float dy = *y - cy;
    float rx = dx * cosf(angle) - dy * sinf(angle);
    float ry = dx * sinf(angle) + dy * cosf(angle);
    *x = cx + rx;
    *y = cy + ry;
}

void shapes_update_and_draw(uint32_t *fb, int w, int h)
{
    int cx = w/2;
    int cy = h/2;
    float max_size = fminf(w,h) * 0.6f;
    
    for(int i=0; i<g_count;){
        bass_shape_t *s = &g_shapes[i];
        
        /* update */
        if(s->scale < 1.0f) s->scale += 0.15f; /* fast growth */
        s->alpha -= 8;
        if(s->alpha < 0) s->alpha = 0;
        s->rotation += s->rot_speed;
        
        if(s->alpha <= 0){
            /* remove by swap */
            g_shapes[i] = g_shapes[--g_count];
            continue;
        }
        
        /* draw with alpha blend simulation */
        float size = max_size * fminf(s->scale, 1.0f);
        float alpha_f = s->alpha / 255.0f;
        uint32_t r = (s->color >> 24) & 0xFF;
        uint32_t g = (s->color >> 16) & 0xFF;
        uint32_t b = (s->color >> 8) & 0xFF;
        r = (uint32_t)(r * alpha_f);
        g = (uint32_t)(g * alpha_f);
        b = (uint32_t)(b * alpha_f);
        uint32_t col = (r<<24)|(g<<16)|(b<<8)|0xFF;
        
        int vx[10], vy[10];
        int n = 0;
        
        switch(s->type){
        case SHAPE_TRIANGLE:
            n = 3;
            for(int j=0; j<3; j++){
                float angle = s->rotation + j * 2.0f * M_PI / 3.0f;
                float x = cx + size * 0.8f * cosf(angle);
                float y = cy + size * 0.8f * sinf(angle);
                vx[j] = (int)x;
                vy[j] = (int)y;
            }
            break;
            
        case SHAPE_DIAMOND:
            n = 4;
            vx[0] = cx; vy[0] = cy - (int)(size*0.8f);
            vx[1] = cx + (int)(size*0.6f); vy[1] = cy;
            vx[2] = cx; vy[2] = cy + (int)(size*0.8f);
            vx[3] = cx - (int)(size*0.6f); vy[3] = cy;
            for(int j=0; j<4; j++){
                float x = vx[j], y = vy[j];
                rotate_point(&x, &y, cx, cy, s->rotation);
                vx[j] = (int)x; vy[j] = (int)y;
            }
            break;
            
        case SHAPE_HEXAGON:
            n = 6;
            for(int j=0; j<6; j++){
                float angle = s->rotation + j * M_PI / 3.0f;
                vx[j] = cx + (int)(size * 0.7f * cosf(angle));
                vy[j] = cy + (int)(size * 0.7f * sinf(angle));
            }
            break;
            
        case SHAPE_STAR:
            n = 10;
            for(int j=0; j<10; j++){
                float angle = s->rotation + j * M_PI / 5.0f;
                float r = (j%2==0) ? size*0.8f : size*0.4f;
                vx[j] = cx + (int)(r * cosf(angle));
                vy[j] = cy + (int)(r * sinf(angle));
            }
            break;
            
        case SHAPE_SQUARE:
            n = 4;
            float half = size * 0.6f;
            vx[0] = cx - (int)half; vy[0] = cy - (int)half;
            vx[1] = cx + (int)half; vy[1] = cy - (int)half;
            vx[2] = cx + (int)half; vy[2] = cy + (int)half;
            vx[3] = cx - (int)half; vy[3] = cy + (int)half;
            for(int j=0; j<4; j++){
                float x = vx[j], y = vy[j];
                rotate_point(&x, &y, cx, cy, s->rotation);
                vx[j] = (int)x; vy[j] = (int)y;
            }
            break;
        }
        
        /* draw outline only (thickness 3) */
        raster_poly(fb, w, h, vx, vy, n, col, false, 3);
        
        ++i;
    }
} 