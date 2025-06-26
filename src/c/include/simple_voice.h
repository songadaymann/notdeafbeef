#ifndef SIMPLE_VOICE_H
#define SIMPLE_VOICE_H

#include <stdint.h>
#include "osc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Simple oscillator voice with exponential decay envelope.
   Supports sine, triangle, square waveforms. */

typedef enum {
    SIMPLE_SINE = 0,
    SIMPLE_TRI  = 1,
    SIMPLE_SQUARE = 2
} simple_wave_t;

typedef struct {
    osc_t osc;
    uint32_t len;
    uint32_t pos;
    float32_t sr;
    float32_t decay;
    float32_t amp;
    simple_wave_t wave;
    float32_t freq;
} simple_voice_t;

void simple_voice_init(simple_voice_t *v, float32_t sr);
void simple_voice_trigger(simple_voice_t *v, float32_t freq, float32_t dur_sec, simple_wave_t wave, float32_t amp, float32_t decay);
void simple_voice_process(simple_voice_t *v, float32_t *L, float32_t *R, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_VOICE_H */ 