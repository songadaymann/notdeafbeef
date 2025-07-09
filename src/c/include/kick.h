#ifndef KICK_H
#define KICK_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float32_t sr;
    uint32_t pos;     /* current sample in envelope; =len when inactive */
    uint32_t len;     /* total samples of sound */
    float32_t env;    /* current envelope value (1→0) */
    float32_t env_coef;/* per-sample decay coefficient */
    float32_t phase;  /* current phase accumulator (radians or table index) */
    float32_t phase_inc; /* per-sample phase increment for table lookup */
    float32_t y_prev;
    float32_t y_prev2;
    float32_t k1;
} kick_t;

/* Initialise kick voice */
void kick_init(kick_t *k, float32_t sr);

/* Start a new kick hit */
void kick_trigger(kick_t *k);

/* Render `n` samples, adding into stereo buffers L/R. */
void kick_process(kick_t *k, float32_t *L, float32_t *R, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* KICK_H */ 