#include "generator.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "fm_presets.h"
#include "euclid.h"

/* Helper for RNG float */
#define RNG_FLOAT(rng) ( (rng_next_u32(rng) >> 8) * (1.0f/16777216.0f) )

volatile float g_block_rms = 0.0f;

// C fallback for generator_build_events_asm - for debugging
static void generator_build_events_c(event_queue_t *q, rng_t *rng, 
                                     const uint8_t *kick_pat, const uint8_t *snare_pat, const uint8_t *hat_pat,
                                     uint32_t step_samples)
{
    eq_init(q);
    
    for(uint32_t step = 0; step < TOTAL_STEPS; step++) {
        uint32_t t = step * step_samples;
        uint32_t bar_step = step % STEPS_PER_BAR;
        
        // Drums
        if(kick_pat[bar_step])  eq_push(q, t, EVT_KICK, 0);
        if(snare_pat[bar_step]) eq_push(q, t, EVT_SNARE, 0);
        if(hat_pat[bar_step])   eq_push(q, t, EVT_HAT, 0);
        
        // Melody at specific positions
        if(bar_step == 0 || bar_step == 8 || bar_step == 16 || bar_step == 24) {
            eq_push(q, t, EVT_MELODY, bar_step/8);
        }
        
        // Mid triggers
        if((bar_step % 4) == 2 || (((bar_step % 4) == 1 || (bar_step % 4) == 3) && RNG_FLOAT(rng) < 0.1f)) {
            eq_push(q, t, EVT_MID, rng_next_u32(rng) % 7);
        }
        
        // Bass at bar start
        if(bar_step == 0) {
            eq_push(q, t, EVT_FM_BASS, 0);
        }
    }
}

// C fallback for generator_rotate_pattern_asm - for debugging
static void generator_rotate_pattern_c(uint8_t *pattern, uint8_t *tmp, uint32_t size, uint32_t rot)
{
    if(rot == 0) return;
    
    // Copy original pattern to tmp
    memcpy(tmp, pattern, size);
    
    // Rotate: pattern[i] = tmp[(i + rot) % size]
    for(uint32_t i = 0; i < size; i++) {
        pattern[i] = tmp[(i + rot) % size];
    }
}

void generator_init(generator_t *g, uint64_t seed)
{
    memset(g, 0, sizeof(generator_t));
    g->rng = rng_seed(seed);

    /* ---- Derive per-run musical variation from seed ---- */
    uint8_t kick_hits  = 2 + (rng_next_u32(&g->rng) % 3);
    uint8_t snare_hits = 1 + (rng_next_u32(&g->rng) % 3);
    uint8_t hat_hits   = 4 + (rng_next_u32(&g->rng) % 5);
    uint8_t preset_offset = rng_next_u32(&g->rng) % 4;
    
    float bpm = 50.0f + (RNG_FLOAT(&g->rng) * 70.0f);
    music_time_init(&g->mt, bpm);
    music_globals_init(&g->music, &g->rng);

    /* ---- Init voices ---- */
    kick_init(&g->kick, SR);
    snare_init(&g->snare, SR, seed ^ 0xABCDEF);
    hat_init(&g->hat, SR,   seed ^ 0x123456);
    melody_init(&g->mel, SR);
    fm_voice_init(&g->mid_fm, SR);
    fm_voice_init(&g->bass_fm, SR);
    simple_voice_init(&g->mid_simple, SR);

    /* ---- Build drum patterns ---- */
    uint8_t kick_pat[STEPS_PER_BAR], snare_pat[STEPS_PER_BAR], hat_pat[STEPS_PER_BAR];
    euclid_pattern(kick_hits, STEPS_PER_BAR, kick_pat);
    euclid_pattern(snare_hits, STEPS_PER_BAR, snare_pat);
    euclid_pattern(hat_hits, STEPS_PER_BAR, hat_pat);

    uint8_t rot = rng_next_u32(&g->rng) % STEPS_PER_BAR;
    if(rot > 0){
        uint8_t tmp[STEPS_PER_BAR];
        /* Phase 5.4: Use C implementation for debugging segfault */
        generator_rotate_pattern_c(kick_pat, tmp, STEPS_PER_BAR, rot);
        generator_rotate_pattern_c(snare_pat, tmp, STEPS_PER_BAR, rot);
        generator_rotate_pattern_c(hat_pat, tmp, STEPS_PER_BAR, rot);
    }
    
    /* ---- Pre-compute event queue ---- */
    /* Phase 5.5: Use C implementation (assembly has infinite loop bug) */
    generator_build_events_c(&g->q, &g->rng, kick_pat, snare_pat, hat_pat, g->mt.step_samples);
    g->event_idx = 0;
    g->step = 0;
    g->pos_in_step = 0;

    /* Debug: count how many EVT_MID events were scheduled */
    uint32_t mid_evt_count = 0;
    for(uint32_t i = 0; i < g->q.count; i++){
        if(g->q.events[i].type == EVT_MID) mid_evt_count++;
    }
    printf("DEBUG: EVT_MID events scheduled = %u\n", mid_evt_count);
    
    /* ---- Init Effects ---- */
#ifdef DELAY_FACTOR_OVERRIDE
    float delay_factor = DELAY_FACTOR_OVERRIDE;
#else
    float delay_factors[] = {2.0f,1.0f,0.5f,0.25f};
    float delay_factor = delay_factors[rng_next_u32(&g->rng)%4];
#endif
    uint32_t delay_samples = (uint32_t)(g->mt.beat_sec * delay_factor * SR);
    if(delay_samples > MAX_DELAY_SAMPLES) delay_samples = MAX_DELAY_SAMPLES;
    delay_init(&g->delay, g->delay_buf, delay_samples);
    printf("DEBUG: After delay_init - buf=%p size=%u idx=%u\n", g->delay.buf, g->delay.size, g->delay.idx);
    printf("DEBUG: LLDB WATCHPOINT ADDRESSES - delay struct at %p, delay.size at %p, delay.idx at %p\n", 
           &g->delay, &g->delay.size, &g->delay.idx);
    /* Limiter tweak: faster attack/release and softer threshold (−0.1 dB) */
    limiter_init(&g->limiter, SR, 0.5f, 50.0f, -0.1f);
}

#ifndef GENERATOR_ASM
void generator_process(generator_t *g, float32_t *L, float32_t *R, uint32_t num_frames)
{
    /* clear visual event flags */
    g->saw_hit = false;
    g->bass_hit = false;

    /* buffers for sub-mixes */
    float32_t Ld[num_frames], Rd[num_frames];
    float32_t Ls[num_frames], Rs[num_frames];
    
    /* Phase 5.3: Use C implementation for debugging */
    memset(Ld, 0, num_frames * sizeof(float32_t));
    memset(Rd, 0, num_frames * sizeof(float32_t));
    memset(Ls, 0, num_frames * sizeof(float32_t));
    memset(Rs, 0, num_frames * sizeof(float32_t));

    uint32_t frames_rem = num_frames;
    uint32_t current_frame = 0;

    while(frames_rem > 0){
        /* Trigger events at the *beginning* of each step */
        if(g->pos_in_step == 0){
            uint32_t t_step_start = g->step * g->mt.step_samples;
            while(g->event_idx < g->q.count && g->q.events[g->event_idx].time == t_step_start){
                event_t *e = &g->q.events[g->event_idx];
                switch(e->type){
                    case EVT_KICK:  kick_trigger(&g->kick); break;
                    case EVT_SNARE: snare_trigger(&g->snare); break;
                    case EVT_HAT:   hat_trigger(&g->hat); break;
                    case EVT_MELODY: {
                        float32_t freq=g->music.root_freq;
                        int deg;
                        switch(e->aux){
                            case 0: freq *= 4.0f; break;
                            case 1: deg=g->music.scale_degrees[rng_next_u32(&g->rng)%(g->music.scale_len-1)+1]; freq *= powf(2.0f, deg/12.0f + 1.0f); break;
                            case 2: freq *= 4.0f; break;
                            case 3: deg=g->music.scale_degrees[rng_next_u32(&g->rng)%(g->music.scale_len-1)+1]; freq *= powf(2.0f, deg/12.0f); break;
                        }
                        melody_trigger(&g->mel, freq, g->mt.beat_sec);
                        g->saw_hit = true;
                        break; }
                    case EVT_MID: {
                        uint8_t idx = e->aux;
                        int deg = g->music.scale_degrees[rng_next_u32(&g->rng)%g->music.scale_len];
                        float32_t freq = g->music.root_freq * powf(2.0f, deg/12.0f + 1.0f);
                        if(idx < 3){
                            simple_wave_t w = (idx==0)?SIMPLE_TRI:(idx==1)?SIMPLE_SINE:SIMPLE_SQUARE;
                            simple_voice_trigger(&g->mid_simple, freq, g->mt.step_sec, w, 0.2f, 6.0f);
                        } else {
                            fm_params_t mid_presets[4] = {FM_PRESET_BELLS, FM_PRESET_CALM, FM_PRESET_QUANTUM, FM_PRESET_PLUCK};
                            fm_params_t p = mid_presets[(idx-3)%4];
                            fm_voice_trigger(&g->mid_fm, freq, g->mt.step_sec + (1.0f/(float32_t)SR), p.ratio, p.index, p.amp, p.decay);
                        }
                        break; }
                    case EVT_FM_BASS: {
                        int deg = g->music.scale_degrees[rng_next_u32(&g->rng)%g->music.scale_len];
                        float32_t freq = g->music.root_freq/4.0f * powf(2.0f, deg/12.0f);
                        uint8_t bass_choice = rng_next_u32(&g->rng) % 3;
                        fm_params_t p;
                        switch(bass_choice){
                            default: case 0: p = FM_BASS_DEFAULT; break;
                            case 1: p = FM_BASS_QUANTUM; break;
                            case 2: p = FM_BASS_PLUCKY;  break;
                        }
                        fm_voice_trigger(&g->bass_fm, freq, g->mt.beat_sec*2, p.ratio, p.index, p.amp, p.decay);
                        g->bass_hit = true;
                        break; }
                }
                g->event_idx++;
            }
        }

        /* How many frames until the next step boundary? */
        uint32_t frames_to_step_boundary = g->mt.step_samples - g->pos_in_step;
        /*
         * FM sustain fix: ensure the first slice of every step processes at most
         * step_samples-1 frames so notes longer than one slice are not fully
         * consumed in a single call (they would then be silent on subsequent
         * slices). Only apply when we are at the very start of a step and have
         * more than one frame left to render.
         */
        if (g->pos_in_step == 0 && frames_to_step_boundary > 1) {
            frames_to_step_boundary -= 1;
        }
        uint32_t frames_to_process = (frames_rem < frames_to_step_boundary) ? frames_rem : frames_to_step_boundary;

        /* Render voices */
        kick_process(&g->kick,   &Ld[current_frame], &Rd[current_frame], frames_to_process);
        snare_process(&g->snare, &Ld[current_frame], &Rd[current_frame], frames_to_process);
        hat_process(&g->hat,     &Ld[current_frame], &Rd[current_frame], frames_to_process);
        melody_process(&g->mel,      &Ls[current_frame], &Rs[current_frame], frames_to_process);
        fm_voice_process(&g->mid_fm, &Ls[current_frame], &Rs[current_frame], frames_to_process);
        fm_voice_process(&g->bass_fm,&Ls[current_frame], &Rs[current_frame], frames_to_process);
        simple_voice_process(&g->mid_simple, &Ls[current_frame], &Rs[current_frame], frames_to_process);

        /* Advance pointers / counters */
        current_frame += frames_to_process;
        frames_rem    -= frames_to_process;
        g->pos_in_step += frames_to_process;

        /* Step boundary reached? */
        if(g->pos_in_step >= g->mt.step_samples){
            g->pos_in_step = 0;
            g->step++;
            if(g->step >= TOTAL_STEPS){
                g->step = 0;
                g->event_idx = 0; /* loop event queue */
            }
        }
    }

    printf("DEBUG: Before delay_process_block - buf=%p size=%u idx=%u n=%u\n", g->delay.buf, g->delay.size, g->delay.idx, num_frames);
    delay_process_block(&g->delay, Ls, Rs, num_frames, 0.45f);

    /* Phase 5.1: Use C implementation for debugging */
    for(uint32_t i = 0; i < num_frames; i++) {
        L[i] = Ld[i] + Ls[i];
        R[i] = Rd[i] + Rs[i];
    }

    limiter_process(&g->limiter, L, R, num_frames);

    /* Phase 5.2: Use C implementation for debugging */
    float sum = 0.0f;
    for(uint32_t i = 0; i < num_frames; i++) {
        sum += L[i] * L[i] + R[i] * R[i];
    }
    g_block_rms = sqrtf(sum / (num_frames * 2));
}
#endif // GENERATOR_ASM 