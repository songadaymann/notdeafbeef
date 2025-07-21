#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "../include/visual_types.h"

// Set a pixel in the buffer with bounds checking
static void set_pixel(uint32_t *pixels, int x, int y, uint32_t color) {
    if (x >= 0 && x < VIS_WIDTH && y >= 0 && y < VIS_HEIGHT) {
        pixels[y * VIS_WIDTH + x] = color;
    }
}

// Clear the entire frame buffer
void clear_frame(uint32_t *pixels, uint32_t color) {
    for (int i = 0; i < VIS_WIDTH * VIS_HEIGHT; i++) {
        pixels[i] = color;
    }
}

// Draw a circle outline (matching pygame.draw.circle)
void draw_circle_outline(uint32_t *pixels, int cx, int cy, int radius, uint32_t color, int thickness) {
    for (int t = 0; t < thickness; t++) {
        int r = radius + t;
        
        // Use Bresenham-style circle algorithm
        int x = 0;
        int y = r;
        int d = 3 - 2 * r;
        
        while (y >= x) {
            // Draw all 8 octants
            set_pixel(pixels, cx + x, cy + y, color);
            set_pixel(pixels, cx - x, cy + y, color);
            set_pixel(pixels, cx + x, cy - y, color);
            set_pixel(pixels, cx - x, cy - y, color);
            set_pixel(pixels, cx + y, cy + x, color);
            set_pixel(pixels, cx - y, cy + x, color);
            set_pixel(pixels, cx + y, cy - x, color);
            set_pixel(pixels, cx - y, cy - x, color);
            
            x++;
            if (d > 0) {
                y--;
                d = d + 4 * (x - y) + 10;
            } else {
                d = d + 4 * x + 6;
            }
        }
    }
}

// Draw a filled circle
void draw_circle_filled(uint32_t *pixels, int cx, int cy, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                set_pixel(pixels, cx + x, cy + y, color);
            }
        }
    }
}

// Draw a polygon outline
void draw_polygon_outline(uint32_t *pixels, pointf_t *points, int num_points, uint32_t color, int thickness) {
    for (int i = 0; i < num_points; i++) {
        int next = (i + 1) % num_points;
        
        // Simple line drawing between consecutive points
        int x1 = (int)points[i].x;
        int y1 = (int)points[i].y;
        int x2 = (int)points[next].x;
        int y2 = (int)points[next].y;
        
        // Bresenham line algorithm
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);
        int sx = x1 < x2 ? 1 : -1;
        int sy = y1 < y2 ? 1 : -1;
        int err = dx - dy;
        
        int x = x1, y = y1;
        while (1) {
            // Draw with thickness
            for (int t = 0; t < thickness; t++) {
                set_pixel(pixels, x + t, y, color);
                set_pixel(pixels, x, y + t, color);
                set_pixel(pixels, x - t, y, color);
                set_pixel(pixels, x, y - t, color);
            }
            
            if (x == x2 && y == y2) break;
            
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }
}

// Helper function to create circle color with hue variation
uint32_t circle_color(float base_hue, float saturation, float value) {
    hsv_t hsv = {base_hue, saturation, value};
    color_t rgb = hsv_to_rgb(hsv);
    return color_to_pixel(rgb);
}

// Forward declare ASCII functions
void draw_ascii_char(uint32_t *pixels, int x, int y, char c, uint32_t color, int alpha);
void draw_ascii_circle(uint32_t *pixels, int cx, int cy, int radius, char fill_char, uint32_t color, int alpha);

// Forward declare glitch functions
char get_glitched_shape_char(char original_char, int x, int y, int frame);

// Draw the centerpiece based on mode and parameters (ASCII version)
void draw_centerpiece(uint32_t *pixels, centerpiece_t *centerpiece, float time, float level, int frame) {
    // Calculate orbit position (matching Python: ang=t*0.2)
    float angle = time * centerpiece->orbit_speed;
    int cx = VIS_WIDTH / 2 + (int)(cosf(angle) * centerpiece->orbit_radius);
    int cy = VIS_HEIGHT / 2 + (int)(sinf(angle) * centerpiece->orbit_radius);
    
    // Calculate base radius (matching Python: base_r=int(30+80*level))
    int base_radius = (int)(30 + 80 * level);
    
    uint32_t color = circle_color(centerpiece->base_hue, 1.0f, 1.0f);
    
    switch (centerpiece->mode) {
        case VIS_MODE_THICK: {
            // Simple thick circle using glitched '@' characters
            char base_char = '@';
            char glitched_char = get_glitched_shape_char(base_char, cx, cy, frame);
            draw_ascii_circle(pixels, cx, cy, base_radius + 10, glitched_char, color, 255);
            break;
        }
            
        case VIS_MODE_RINGS: {
            // Multi-layered concentric rings using glitched characters
            char ring_chars[] = {'O', '*', '+'};
            for (int k = 0; k < 3; k++) {
                int ring_radius = base_radius + k * 15 + (int)(10 * sinf(time + k));
                float hue = fmod(centerpiece->base_hue + 0.05f * k, 1.0f);
                uint32_t ring_color = circle_color(hue, 1.0f, 1.0f);
                
                // Apply glitch to ring character
                char glitched_char = get_glitched_shape_char(ring_chars[k], cx + k*10, cy + k*10, frame + k*5);
                draw_ascii_circle(pixels, cx, cy, ring_radius, glitched_char, ring_color, 255);
            }
            break;
        }
            
        case VIS_MODE_POLY: {
            // Rotating polygon using glitched '#' characters at vertices
            int num_sides = 4 + centerpiece->bpm / 30;
            if (num_sides > 6) num_sides = 6;
            
            for (int i = 0; i < num_sides; i++) {
                float vertex_angle = time + i * 2.0f * M_PI / num_sides;
                int px = cx + (int)(cosf(vertex_angle) * base_radius);
                int py = cy + (int)(sinf(vertex_angle) * base_radius);
                
                // Glitch the vertex character
                char vertex_char = get_glitched_shape_char('#', px, py, frame + i);
                draw_ascii_char(pixels, px, py, vertex_char, color, 255);
                
                // Connect vertices with lines of glitched characters
                float next_angle = time + ((i + 1) % num_sides) * 2.0f * M_PI / num_sides;
                int next_px = cx + (int)(cosf(next_angle) * base_radius);
                int next_py = cy + (int)(sinf(next_angle) * base_radius);
                
                // Draw line between vertices using glitched '-' and '|'
                int dx = next_px - px;
                int dy = next_py - py;
                int steps = (int)sqrtf(dx*dx + dy*dy) / 8; // 8 pixels per character
                
                for (int step = 0; step <= steps; step++) {
                    float t = (float)step / steps;
                    int line_x = px + (int)(t * dx);
                    int line_y = py + (int)(t * dy);
                    char base_line_char = abs(dx) > abs(dy) ? '-' : '|';
                    char glitched_line_char = get_glitched_shape_char(base_line_char, line_x, line_y, frame + step);
                    draw_ascii_char(pixels, line_x, line_y, glitched_line_char, color, 200);
                }
            }
            break;
        }
        
        case VIS_MODE_LISSA: {
            // Lissajous figure-8 pattern using glitched characters
            char lissa_chars[] = {'*', '+', 'x', '~'};
            for (int i = 0; i < 120; i++) {
                float phi = i * 2.0f * M_PI / 120.0f;
                int x = cx + (int)(base_radius * sinf(2 * phi + time));
                int y = cy + (int)(base_radius * sinf(3 * phi));
                char base_char = lissa_chars[i % 4];
                char glitched_char = get_glitched_shape_char(base_char, x, y, frame + i);
                draw_ascii_char(pixels, x, y, glitched_char, color, 255);
            }
            break;
        }
    }
}
