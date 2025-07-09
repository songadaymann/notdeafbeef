#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "music_time.h"
#include "music_defs.h"
#include "rand.h"
#include "kick.h"
#include "snare.h"
#include "hat.h"
#include "melody.h"
#include "fm_voice.h"
#include "simple_voice.h"
#include "delay.h"
#include "limiter.h"
#include "event_queue.h"

#define MAX_DELAY_SAMPLES 106000

typedef struct {
    music_time_t mt;
    music_globals_t music;
    rng_t rng;

    kick_t kick;
    snare_t snare;
    hat_t hat;
    melody_t mel;
    fm_voice_t mid_fm;
    fm_voice_t bass_fm;
    simple_voice_t mid_simple;

    event_queue_t q;
    uint32_t event_idx;
    uint32_t step;
    uint32_t pos_in_step; /* samples into current step */

    delay_t delay;
    limiter_t limiter;
    
    float32_t delay_buf[MAX_DELAY_SAMPLES * 2];

    /* visual event flags */
    bool saw_hit;      /* set when saw melody triggers */
    bool bass_hit;     /* set when bass triggers */

} generator_t;

void generator_init(generator_t *g, uint64_t seed);
void generator_process(generator_t *g, float32_t *L, float32_t *R, uint32_t num_frames);
void generator_process_voices(generator_t *g, float32_t *Ld, float32_t *Rd,
                              float32_t *Ls, float32_t *Rs, uint32_t num_frames);

/* Phase 5 - Pure Assembly Orchestration Functions */
void generator_mix_buffers_asm(float32_t *L, float32_t *R, 
                               const float32_t *Ld, const float32_t *Rd,
                               const float32_t *Ls, const float32_t *Rs,
                               uint32_t num_frames);

float generator_compute_rms_asm(const float32_t *L, const float32_t *R, 
                                uint32_t num_frames);

void generator_clear_buffers_asm(float32_t *Ld, float32_t *Rd, 
                                 float32_t *Ls, float32_t *Rs,
                                 uint32_t num_frames);

void generator_rotate_pattern_asm(uint8_t *pattern, uint8_t *tmp, 
                                  uint32_t size, uint32_t rot);

void generator_build_events_asm(event_queue_t *q, rng_t *rng, 
                               const uint8_t *kick_pat, const uint8_t *snare_pat, const uint8_t *hat_pat,
                               uint32_t step_samples);

void generator_trigger_step(generator_t *g);

extern volatile float g_block_rms;

#endif /* GENERATOR_H */ 