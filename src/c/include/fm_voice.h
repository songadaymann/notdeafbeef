#ifndef FM_VOICE_H
#define FM_VOICE_H

#include <stdint.h>
#include "osc.h"

typedef struct {
    /* runtime params */
    float32_t sr;
    float32_t carrier_freq;
    float32_t ratio;   /* modulator/carrier */
    float32_t index0;  /* starting index */
    float32_t amp;
    float32_t decay;

    /* state */
    uint32_t len;  /* total samples */
    uint32_t pos;  /* current position */
    float32_t carrier_phase;
    float32_t mod_phase;
} fm_voice_t;

void fm_voice_init(fm_voice_t *v, float32_t sr);
void fm_voice_trigger(fm_voice_t *v, float32_t carrier_freq, float32_t duration_sec, float32_t ratio, float32_t index, float32_t amp, float32_t decay);
void fm_voice_process(fm_voice_t *v, float32_t *L, float32_t *R, uint32_t n);

#endif /* FM_VOICE_H */ 