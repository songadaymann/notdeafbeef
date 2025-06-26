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