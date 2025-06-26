#ifndef MUSIC_TIME_H
#define MUSIC_TIME_H

#include <stdint.h>

/* Global musical timing constants for the segment renderer. */

#define SR 44100u
#define STEPS_PER_BEAT 4u          /* 16th notes */
#define BARS_PER_SEG 2u
#define STEPS_PER_BAR (4u * STEPS_PER_BEAT)
#define TOTAL_STEPS   (BARS_PER_SEG * STEPS_PER_BAR)

typedef struct {
    float bpm;
    float beat_sec;
    float step_sec;
    uint32_t step_samples;
    float seg_sec;
    uint32_t seg_frames;
} music_time_t;

static inline void music_time_init(music_time_t *t, float bpm)
{
    t->bpm = bpm;
    t->beat_sec = 60.0f / t->bpm;
    t->step_sec = t->beat_sec / (float)STEPS_PER_BEAT;
    t->step_samples = (uint32_t)(t->step_sec * SR + 0.5f);
    t->seg_sec = t->step_sec * (float)TOTAL_STEPS;
    t->seg_frames = (uint32_t)(t->seg_sec * SR + 0.5f);
}

#endif /* MUSIC_TIME_H */ 