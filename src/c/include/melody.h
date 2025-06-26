#ifndef MELODY_H
#define MELODY_H

#include <stdint.h>
#include "osc.h"

typedef struct {
    osc_t osc;
    uint32_t pos;
    uint32_t len;
    float32_t sr;
    float32_t freq;
} melody_t;

void melody_init(melody_t *m, float32_t sr);
void melody_trigger(melody_t *m, float32_t freq, float32_t dur_sec);
void melody_process(melody_t *m, float32_t *L, float32_t *R, uint32_t n);

#endif /* MELODY_H */ 