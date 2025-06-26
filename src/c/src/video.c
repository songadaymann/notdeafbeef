#include "video.h"
#ifdef float32_t
#undef float32_t
#endif
#include <SDL.h>
#include <stdlib.h>

typedef struct {
    SDL_Window   *win;
    SDL_Renderer *ren;
    SDL_Texture  *tex;
    uint32_t      fps_interval_ms;
    uint32_t      last_ticks;
    int           width;
    int           height;
    uint32_t     *fb;
} video_state_t;

static video_state_t g;

int video_init(int width, int height, int fps, bool vsync)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0){
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    uint32_t flags = SDL_RENDERER_ACCELERATED | (vsync?SDL_RENDERER_PRESENTVSYNC:0);
    g.win = SDL_CreateWindow("Euclid Visualiser", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width, height, SDL_WINDOW_SHOWN);
    if(!g.win){ SDL_Log("CreateWindow failed: %s", SDL_GetError()); return 1; }

    g.ren = SDL_CreateRenderer(g.win, -1, flags);
    if(!g.ren){ SDL_Log("CreateRenderer failed: %s", SDL_GetError()); return 1; }

    g.width = width; g.height = height;
    g.fb = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    if(!g.fb){ SDL_Log("malloc framebuffer failed"); return 1; }

    g.tex = SDL_CreateTexture(g.ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                              width, height);
    if(!g.tex){ SDL_Log("CreateTexture failed: %s", SDL_GetError()); return 1; }

    g.fps_interval_ms = (fps>0)? (1000u / (uint32_t)fps) : 0u;
    g.last_ticks = SDL_GetTicks();
    return 0;
}

bool video_frame_begin(void)
{
    /* Frame pacing */
    if(g.fps_interval_ms){
        uint32_t now = SDL_GetTicks();
        uint32_t elapsed = now - g.last_ticks;
        if(elapsed < g.fps_interval_ms){
            SDL_Delay(g.fps_interval_ms - elapsed);
        }
        g.last_ticks = SDL_GetTicks();
    }

    /* Event polling */
    SDL_Event ev;
    while(SDL_PollEvent(&ev)){
        if(ev.type == SDL_QUIT){
            return false; /* request close */
        }
    }

    /* nothing else here, draw into framebuffer in caller */
    return true;
}

void video_frame_end(void)
{
    /* upload framebuffer to texture & present */
    SDL_UpdateTexture(g.tex, NULL, g.fb, g.width * sizeof(uint32_t));
    SDL_RenderClear(g.ren);
    SDL_RenderCopy(g.ren, g.tex, NULL, NULL);
    SDL_RenderPresent(g.ren);
}

void video_shutdown(void)
{
    if(g.ren) SDL_DestroyRenderer(g.ren);
    if(g.tex) SDL_DestroyTexture(g.tex);
    if(g.win) SDL_DestroyWindow(g.win);
    free(g.fb);
    SDL_Quit();
}

uint32_t* video_get_framebuffer(void){ return g.fb; }
int video_get_width(void){ return g.width; }
int video_get_height(void){ return g.height; } 