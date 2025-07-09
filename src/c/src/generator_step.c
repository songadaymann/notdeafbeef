#include "generator.h"
#include "fm_presets.h"
#include "fm_voice.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_MID_LOG 1

/* Helper for RNG float (copied from generator.c) */
#define RNG_FLOAT(rng) ( (rng_next_u32(rng) >> 8) * (1.0f/16777216.0f) )

/* Global debug counter for mid-FM triggers */
int g_mid_trigger_count = 0;

void generator_trigger_step(generator_t *g)
{
    /* Only act at the very start of a step */
    if(g->pos_in_step != 0) return;

    uint32_t t_step_start = g->step * g->mt.step_samples;

    while(g->event_idx < g->q.count && g->q.events[g->event_idx].time == t_step_start){
        event_t *e = &g->q.events[g->event_idx];
        printf("TRIGGER type=%u aux=%u step=%u pos=%u\n", e->type, e->aux, g->step, g->pos_in_step);
        switch(e->type){
            case EVT_KICK:
                kick_trigger(&g->kick);
                break;
            case EVT_SNARE:
                snare_trigger(&g->snare);
                break;
            case EVT_HAT:
                hat_trigger(&g->hat);
                break;
            case EVT_MELODY: {
                float32_t freq = g->music.root_freq;
                int deg;
                switch(e->aux){
                    case 0:
                        freq *= 4.0f;
                        break;
                    case 1:
                        deg = g->music.scale_degrees[rng_next_u32(&g->rng) % (g->music.scale_len - 1) + 1];
                        freq *= powf(2.0f, deg / 12.0f + 1.0f);
                        break;
                    case 2:
                        freq *= 4.0f;
                        break;
                    case 3:
                        deg = g->music.scale_degrees[rng_next_u32(&g->rng) % (g->music.scale_len - 1) + 1];
                        freq *= powf(2.0f, deg / 12.0f);
                        break;
                }
                melody_trigger(&g->mel, freq, g->mt.beat_sec);
                g->saw_hit = true;
                break; }
            case EVT_MID: {
                /* TEMP DEBUG: Log each mid trigger */
#ifdef DEBUG_MID_LOG
                printf("MID TRIGGER step=%u aux=%u pos=%u\n", g->step, e->aux, g->pos_in_step);
#endif
                g_mid_trigger_count++; /* count how many actually fire */
                uint8_t idx = e->aux;
                int deg = g->music.scale_degrees[rng_next_u32(&g->rng) % g->music.scale_len];
                float32_t freq = g->music.root_freq * powf(2.0f, deg / 12.0f + 1.0f);
                if(idx < 3){
                    simple_wave_t w = (idx == 0) ? SIMPLE_TRI : (idx == 1) ? SIMPLE_SINE : SIMPLE_SQUARE;
                    simple_voice_trigger(&g->mid_simple, freq, g->mt.step_sec, w, 0.2f, 6.0f);
                } else {
                    fm_params_t mid_presets[4] = {FM_PRESET_BELLS, FM_PRESET_CALM, FM_PRESET_QUANTUM, FM_PRESET_PLUCK};
                    fm_params_t p = mid_presets[(idx - 3) % 4];
                    fm_voice_trigger(&g->mid_fm, freq, g->mt.step_sec + (1.0f/ (float32_t)SR), p.ratio, p.index, p.amp, p.decay);
                }
                break; }
            case EVT_FM_BASS: {
                int deg = g->music.scale_degrees[rng_next_u32(&g->rng) % g->music.scale_len];
                float32_t freq = g->music.root_freq / 4.0f * powf(2.0f, deg / 12.0f);
                uint8_t bass_choice = rng_next_u32(&g->rng) % 3;
                fm_params_t p;
                switch(bass_choice){
                    default:
                    case 0:
                        p = FM_BASS_DEFAULT;
                        break;
                    case 1:
                        p = FM_BASS_QUANTUM;
                        break;
                    case 2:
                        p = FM_BASS_PLUCKY;
                        break;
                }
                fm_voice_trigger(&g->bass_fm, freq, g->mt.beat_sec * 2, p.ratio, p.index, p.amp, p.decay);
                g->bass_hit = true;
                break; }
        }
        g->event_idx++;
    }

#ifdef DEBUG_MID_LOG
    printf("TRIGGER_STEP END event_idx=%u step=%u pos=%u\n", g->event_idx, g->step, g->pos_in_step);
#endif
}

/*------------------------------------------------------------------
 * Block-level helper: process all voices for a block of n frames
 * writing into scratch buffers (Ld/Rd/Ls/Rs).
 *------------------------------------------------------------------*/
void generator_process_voices(generator_t *g, float32_t *Ld, float32_t *Rd,
                               float32_t *Ls, float32_t *Rs, uint32_t n)
{
    static int dbg_count=0;
    /* Drums: call even when ASM versions are linked – symbol resolves to
       _kick_process/_snare_process/_hat_process in assembly builds, or C
       implementation otherwise. */
    kick_process(&g->kick,   Ld, Rd, n);
    snare_process(&g->snare, Ld, Rd, n);
    hat_process(&g->hat,     Ld, Rd, n);

#ifndef MELODY_ASM
    melody_process(&g->mel,         Ls, Rs, n);
#endif
#ifndef FM_VOICE_ASM
    fm_voice_process(&g->mid_fm,    Ls, Rs, n);
#endif

    /* Removed early return – allow remaining voices to process */

#if 0
    /* RMS probe & voice sum debug – disabled for release build (unaligned NEON loads crash) */
#endif
    fm_voice_process(&g->bass_fm,   Ls, Rs, n);
    simple_voice_process(&g->mid_simple, Ls, Rs, n);
#if 0
    /* Disabled additional debug prints */
#endif
} 