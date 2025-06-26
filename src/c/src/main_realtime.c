#include "coreaudio.h"
#include "generator.h"
#include "video.h"
#include "raster.h"
#include "terrain.h"
#include "particles.h"
#include "shapes.h"
#include "crt_fx.h"
#include <stdio.h>
#include <unistd.h> // for sleep
#include <stdlib.h> // for strtoull
#include <stdbool.h>
#include <math.h>

static generator_t g_generator;

void audio_render_callback(float* buffer, uint32_t num_frames, void* user_data)
{
    float L[num_frames], R[num_frames];
    generator_process(&g_generator, L, R, num_frames);
    for(uint32_t i=0; i<num_frames; ++i){
        buffer[i*2]   = L[i];
        buffer[i*2+1] = R[i];
    }
}

int main(int argc, char **argv)
{
    uint64_t seed = 0xCAFEBABEULL;
    if(argc > 1) {
        seed = strtoull(argv[1], NULL, 0);
    }

    generator_init(&g_generator, seed);
    terrain_init(seed);
    particles_init();
    shapes_init();

    /* init CRT effects */
    crt_fx_t crt_fx;
    crt_fx_init(&crt_fx, seed, 800, 600);

    if(audio_init(SR, 512, audio_render_callback, NULL) != 0){
        fprintf(stderr, "Audio init failed\n");
        return 1;
    }

    printf("Playing with seed 0x%llx. Close the window to quit.\n", (unsigned long long)seed);

    /* show CRT effect levels */
    printf("CRT FX: persist=%.2f, scan=%d, chroma=%d, noise=%d\n",
           crt_fx.persistence, crt_fx.scanline_alpha, crt_fx.chroma_shift, crt_fx.noise_pixels);
    printf("        jitter=%.1f, drops=%.2f, bleed=%.2f\n",
           crt_fx.jitter_amount, crt_fx.frame_drop_chance, crt_fx.color_bleed);

    /* --- Start audio & video --- */
    audio_start();
    if(video_init(800, 600, 30, true) != 0){
        fprintf(stderr, "Video init failed\n");
        return 1;
    }

    bool running = true;
    uint32_t *fb = video_get_framebuffer();
    int vw = video_get_width();
    int vh = video_get_height();
    float base_hue = 0.0f;
    float angle=0.0f;
    int frame=0;
    while(running){
        running = video_frame_begin();
        /* clear */
        raster_clear(fb, vw, vh, 0x000000FF); /* black, alpha 255 */

        /* orbiting circle driven by RMS */
        float level = g_block_rms; /* 0..1 */
        int radius = 30 + (int)(80.0f * level);
        int cx = vw/2 + (int)(cosf(angle)* (vw/4));
        int cy = vh/2 + (int)(sinf(angle)* (vh/4));
        /* filled circle background */
        raster_fill_circle(fb, vw, vh, cx, cy, radius, 0x005500FF);
        /* outlined ring */
        raster_circle(fb, vw, vh, cx, cy, radius+10, 0x00FF00FF, 4);

        /* draw scrolling floor */
        terrain_draw(fb, vw, vh, frame);

        /* bass hit shapes (behind floor) */
        shapes_update_and_draw(fb, vw, vh);

        /* spawn particles on saw hits */
        if(g_generator.saw_hit){
            float cx = vw * 0.3f + (rand() % (int)(vw * 0.4f));
            float cy = vh * 0.2f + (rand() % (int)(vh * 0.3f));
            /* color with slight hue variation from base */
            float hue = (float)(rand() % 360) / 360.0f;
            uint8_t r = (uint8_t)(127 + 127 * cosf(hue * 2 * M_PI));
            uint8_t g = (uint8_t)(127 + 127 * cosf((hue + 0.33f) * 2 * M_PI));
            uint8_t b = (uint8_t)(127 + 127 * cosf((hue + 0.66f) * 2 * M_PI));
            uint32_t color = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            particles_spawn_burst(cx, cy, 20, color);
        }

        /* spawn bass shapes on bass hits */
        if(g_generator.bass_hit){
            shape_type_t types[] = {SHAPE_TRIANGLE, SHAPE_DIAMOND, SHAPE_HEXAGON, SHAPE_STAR, SHAPE_SQUARE};
            shape_type_t type = types[rand() % 5];
            /* color variation */
            float hue = (float)(rand() % 360) / 360.0f;
            uint8_t r = (uint8_t)(200 + 55 * cosf(hue * 2 * M_PI));
            uint8_t g = (uint8_t)(200 + 55 * cosf((hue + 0.33f) * 2 * M_PI));
            uint8_t b = (uint8_t)(200 + 55 * cosf((hue + 0.66f) * 2 * M_PI));
            uint32_t color = (r << 24) | (g << 16) | (b << 8) | 0xFF;
            shapes_spawn(type, color);
        }

        particles_update_and_draw(fb, vw, vh);

        /* apply CRT post-processing effects */
        crt_fx_apply(&crt_fx, fb, vw, vh, frame);

        /* jitter effect (screen shake) */
        if(crt_fx.jitter_amount > 0.01f && (rand() % 100) < 30){
            int jx = (int)(-crt_fx.jitter_amount + (rand() % (int)(crt_fx.jitter_amount * 2)));
            int jy = (int)(-crt_fx.jitter_amount + (rand() % (int)(crt_fx.jitter_amount * 2)));
            /* shift framebuffer content */
            uint32_t *temp = (uint32_t*)malloc(vw * vh * sizeof(uint32_t));
            memcpy(temp, fb, vw * vh * sizeof(uint32_t));
            raster_clear(fb, vw, vh, 0x000000FF);
            for(int y = 0; y < vh; y++){
                for(int x = 0; x < vw; x++){
                    int src_x = x - jx;
                    int src_y = y - jy;
                    if(src_x >= 0 && src_x < vw && src_y >= 0 && src_y < vh){
                        fb[y * vw + x] = temp[src_y * vw + src_x];
                    }
                }
            }
            free(temp);
        }

        angle += 0.02f;

        /* frame drop effect (skip presenting occasionally) */
        if(crt_fx.frame_drop_chance < 0.01f || (rand() % 1000) > (int)(crt_fx.frame_drop_chance * 1000)){
            video_frame_end();
        }

        frame++;
    }

    video_shutdown();
    crt_fx_cleanup(&crt_fx);
    audio_stop();
    return 0;
} 