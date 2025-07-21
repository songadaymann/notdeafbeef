#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include "../include/audio_bridge.h"
#include "../include/visual_types.h"

// Forward declarations
void clear_frame(uint32_t *pixels, uint32_t color);
void draw_centerpiece(uint32_t *pixels, centerpiece_t *centerpiece, float time, float level, int frame);
void init_centerpiece(centerpiece_t *centerpiece, uint32_t seed, int bpm);
void init_degradation_effects(degradation_t *effects, uint32_t seed);
void init_terrain(uint32_t seed, float base_hue);
void draw_terrain(uint32_t *pixels, int frame);
void init_particles(void);
void update_particles(float elapsed_ms, float step_sec, float base_hue);
void draw_particles(uint32_t *pixels);
void init_glitch_system(uint32_t seed, float intensity);
void update_glitch_intensity(float new_intensity);
void init_bass_hits(void);
void update_bass_hits(float elapsed_ms, float step_sec, float base_hue, uint32_t seed);
void draw_bass_hits(uint32_t *pixels, int frame);
bool load_wav_file(const char *filename);
float get_audio_rms_for_frame(int frame);
float get_audio_bpm(void);
float get_max_rms(void);
bool is_audio_finished(int frame);
void print_audio_info(void);
void cleanup_audio_data(void);
void start_audio_playback(void);
void stop_audio_playback(void);

#define FRAME_TIME_MS (1000 / VIS_FPS)

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    visual_context_t visual;
    bool running;
} VisualContext;

static VisualContext ctx = {0};

static bool init_sdl(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    ctx.window = SDL_CreateWindow(
        "NotDeafBeef Visual",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        VIS_WIDTH, VIS_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (!ctx.window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED);
    if (!ctx.renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return false;
    }

    ctx.texture = SDL_CreateTexture(
        ctx.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        VIS_WIDTH, VIS_HEIGHT
    );
    
    if (!ctx.texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return false;
    }

    ctx.visual.pixels = calloc(VIS_WIDTH * VIS_HEIGHT, sizeof(uint32_t));
    if (!ctx.visual.pixels) {
        fprintf(stderr, "Failed to allocate pixel buffer\n");
        return false;
    }

    // Load and analyze audio file
    const char *wav_file = "src/c/seed_0xcafebabe.wav";
    if (!load_wav_file(wav_file)) {
        printf("Failed to load audio file: %s\n", wav_file);
        printf("Falling back to test mode\n");
        // Fallback to test parameters
        ctx.visual.bpm = 120;
    } else {
        printf("Successfully loaded audio file!\n");
        print_audio_info();
        ctx.visual.bpm = (int)get_audio_bpm();
    }
    
    // Initialize visual system with real audio parameters
    ctx.visual.seed = 0xcafebabe;  // Match the WAV file seed
    ctx.visual.frame = 0;
    ctx.visual.time = 0.0f;
    ctx.visual.step_sec = 60.0f / ctx.visual.bpm / 4.0f;  // 16th note duration
    
    init_centerpiece(&ctx.visual.centerpiece, ctx.visual.seed, ctx.visual.bpm);
    init_degradation_effects(&ctx.visual.effects, ctx.visual.seed);
    
    // Initialize terrain system
    init_terrain(ctx.visual.seed, ctx.visual.centerpiece.base_hue);
    
    // Initialize particle system
    init_particles();
    
    // Initialize glitch system with medium intensity
    init_glitch_system(ctx.visual.seed, 0.6f);
    
    // Initialize bass hit system
    init_bass_hits();
    
    // Start audio playback
    start_audio_playback();

    ctx.running = true;
    return true;
}

static void cleanup_sdl(void) {
    stop_audio_playback();
    cleanup_audio_data();
    if (ctx.visual.pixels) free(ctx.visual.pixels);
    if (ctx.texture) SDL_DestroyTexture(ctx.texture);
    if (ctx.renderer) SDL_DestroyRenderer(ctx.renderer);
    if (ctx.window) SDL_DestroyWindow(ctx.window);
    SDL_Quit();
}

static void handle_events(void) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ctx.running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    ctx.running = false;
                }
                break;
        }
    }
}

static void render_frame(void) {
    // Clear to black
    clear_frame(ctx.visual.pixels, 0xFF000000);
    
    // Update time and frame
    ctx.visual.time = ctx.visual.frame / (float)VIS_FPS;
    
    // Get real audio level from WAV analysis
    float raw_rms = get_audio_rms_for_frame(ctx.visual.frame);
    float max_rms = get_max_rms();
    float normalized_level = (max_rms > 0.0f) ? (raw_rms / max_rms) : 0.0f;
    
    // Ensure level is in reasonable range for visuals
    float audio_level = fmaxf(0.0f, fminf(1.0f, normalized_level));
    
    // Audio loops automatically, so no need to check for finish
    
    // Calculate elapsed time in segment (for particle timing) with looping
    float segment_duration_ms = 9220.0f; // ~9.22 seconds from our audio
    float elapsed_ms = fmod(ctx.visual.time * 1000.0f, segment_duration_ms);
    
    // Update particles (handles explosions and physics)
    update_particles(elapsed_ms, ctx.visual.step_sec, ctx.visual.centerpiece.base_hue);
    
    // Update bass hits (handles spawning and animation)
    update_bass_hits(elapsed_ms, ctx.visual.step_sec, ctx.visual.centerpiece.base_hue, ctx.visual.seed);
    
    // Update glitch intensity based on audio level (more glitch = higher energy)
    float glitch_intensity = 0.3f + audio_level * 0.7f; // 0.3 to 1.0
    update_glitch_intensity(glitch_intensity);
    
    // Draw orbiting centerpiece
    draw_centerpiece(ctx.visual.pixels, &ctx.visual.centerpiece, ctx.visual.time, audio_level, ctx.visual.frame);
    
    // Draw bass hits (behind terrain but in front of centerpiece)
    draw_bass_hits(ctx.visual.pixels, ctx.visual.frame);
    
    // Draw terrain (behind particles)
    draw_terrain(ctx.visual.pixels, ctx.visual.frame);
    
    // Draw particles on top
    draw_particles(ctx.visual.pixels);
    
    // TODO: Add other visual elements
    // - Audio-reactive elements
    // - Post-processing effects
    
    // Update texture with pixel buffer
    SDL_UpdateTexture(ctx.texture, NULL, ctx.visual.pixels, VIS_WIDTH * sizeof(uint32_t));
    
    // Render to screen
    SDL_RenderClear(ctx.renderer);
    SDL_RenderCopy(ctx.renderer, ctx.texture, NULL, NULL);
    SDL_RenderPresent(ctx.renderer);
    
    ctx.visual.frame++;
}

static void main_loop(void) {
    uint32_t frame_start;
    uint32_t frame_time;
    
    while (ctx.running) {
        frame_start = SDL_GetTicks();
        
        handle_events();
        
        // TODO: Get audio timing and sync visuals
        // unsigned int audio_time = get_audio_time_ms();
        // float rms = get_rms_level(frame_idx);
        
        render_frame();
        
        // Frame rate limiting
        frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frame_time);
        }
    }
}

int main(int argc, char *argv[]) {
    printf("NotDeafBeef Visual System\n");
    printf("Resolution: %dx%d @ %d FPS\n", VIS_WIDTH, VIS_HEIGHT, VIS_FPS);
    
    if (!init_sdl()) {
        cleanup_sdl();
        return 1;
    }
    
    printf("SDL2 initialized successfully\n");
    printf("Press ESC to exit\n");
    
    main_loop();
    
    cleanup_sdl();
    printf("Visual system shutdown complete\n");
    return 0;
}
