#include "crt_fx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

void crt_fx_init(crt_fx_t *fx, uint64_t seed, int w, int h)
{
    /* allocate persistence buffer */
    fx->prev_frame = (uint32_t*)calloc(w * h, sizeof(uint32_t));
    
    /* seed-based randomization of effect levels */
    fx->rng = rng_seed(seed ^ 0xDE5A7ULL);
    
    fx->persistence = 0.3f + rng_next_float(&fx->rng) * 0.6f;
    fx->scanline_alpha = (int)(rng_next_float(&fx->rng) * 200.0f);
    fx->chroma_shift = (int)(rng_next_float(&fx->rng) * 5.0f);
    fx->noise_pixels = (int)(rng_next_float(&fx->rng) * 300.0f);
    fx->jitter_amount = rng_next_float(&fx->rng) * 3.0f;
    fx->frame_drop_chance = rng_next_float(&fx->rng) * 0.1f;
    fx->color_bleed = rng_next_float(&fx->rng) * 0.3f;
}

void crt_fx_cleanup(crt_fx_t *fx)
{
    free(fx->prev_frame);
}

/* blend two pixels with alpha */
static inline uint32_t blend_alpha(uint32_t dst, uint32_t src, float alpha)
{
    uint8_t dr = (dst >> 24) & 0xFF;
    uint8_t dg = (dst >> 16) & 0xFF;
    uint8_t db = (dst >> 8) & 0xFF;
    
    uint8_t sr = (src >> 24) & 0xFF;
    uint8_t sg = (src >> 16) & 0xFF;
    uint8_t sb = (src >> 8) & 0xFF;
    
    uint8_t r = (uint8_t)(dr * (1.0f - alpha) + sr * alpha);
    uint8_t g = (uint8_t)(dg * (1.0f - alpha) + sg * alpha);
    uint8_t b = (uint8_t)(db * (1.0f - alpha) + sb * alpha);
    
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}

void crt_fx_apply(crt_fx_t *fx, uint32_t *fb, int w, int h, int frame)
{
    /* 1. Persistence (ghost trails) - blend with previous frame */
    if(fx->persistence > 0.01f){
        for(int i = 0; i < w*h; i++){
            fb[i] = blend_alpha(fx->prev_frame[i], fb[i], 1.0f - fx->persistence);
        }
    }
    
    /* Save current frame for next time */
    memcpy(fx->prev_frame, fb, w * h * sizeof(uint32_t));
    
    /* 2. Scanlines - darken every other row */
    if(fx->scanline_alpha > 0){
        float alpha = fx->scanline_alpha / 255.0f;
        for(int y = 0; y < h; y += 2){
            for(int x = 0; x < w; x++){
                int idx = y * w + x;
                uint32_t p = fb[idx];
                uint8_t r = ((p >> 24) & 0xFF) * (1.0f - alpha);
                uint8_t g = ((p >> 16) & 0xFF) * (1.0f - alpha);
                uint8_t b = ((p >> 8) & 0xFF) * (1.0f - alpha);
                fb[idx] = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            }
        }
    }
    
    /* 3. Chromatic aberration - shift red/blue channels */
    if(fx->chroma_shift > 0){
        /* temporary buffer for shifted channels */
        uint32_t *temp = (uint32_t*)malloc(w * h * sizeof(uint32_t));
        memcpy(temp, fb, w * h * sizeof(uint32_t));
        
        int shift = (frame % 30 < 15) ? fx->chroma_shift : -fx->chroma_shift;
        
        /* shift red channel right */
        for(int y = 0; y < h; y++){
            for(int x = 0; x < w; x++){
                int src_x = x - shift;
                if(src_x >= 0 && src_x < w){
                    uint8_t r = (temp[y * w + src_x] >> 24) & 0xFF;
                    fb[y * w + x] = (fb[y * w + x] & 0x00FFFFFF) | (r << 24);
                }
            }
        }
        
        /* shift blue channel left */
        for(int y = 0; y < h; y++){
            for(int x = 0; x < w; x++){
                int src_x = x + shift;
                if(src_x >= 0 && src_x < w){
                    uint8_t b = (temp[y * w + src_x] >> 8) & 0xFF;
                    fb[y * w + x] = (fb[y * w + x] & 0xFFFF00FF) | (b << 8);
                }
            }
        }
        
        free(temp);
    }
    
    /* 4. Color bleed (horizontal blur) */
    if(fx->color_bleed > 0.01f){
        for(int y = 0; y < h; y++){
            for(int x = 1; x < w-1; x++){
                int idx = y * w + x;
                uint32_t left = fb[idx - 1];
                uint32_t right = fb[idx + 1];
                fb[idx] = blend_alpha(fb[idx], blend_alpha(left, right, 0.5f), fx->color_bleed);
            }
        }
    }
    
    /* 5. Random pixel noise */
    for(int i = 0; i < fx->noise_pixels; i++){
        int x = rng_next_u32(&fx->rng) % w;
        int y = rng_next_u32(&fx->rng) % h;
        uint8_t val = rng_next_u32(&fx->rng) & 0xFF;
        fb[y * w + x] = (val << 24) | (val << 16) | (val << 8) | 0xFF;
    }
    
    /* 6. Jitter (whole screen offset) - handled in main loop */
    /* 7. Frame drops - handled in main loop */
} 