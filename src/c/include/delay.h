#ifndef DELAY_H
#define DELAY_H

#include <stdint.h>
#include <string.h>

typedef struct {
    float32_t *buf;      /* interleaved stereo buffer, length = size*2 */
    uint32_t size;   /* delay in samples */
    uint32_t idx;    /* write/read index */
} delay_t;

static inline void delay_init(delay_t *d, float32_t *storage, uint32_t size)
{
    d->buf = storage; d->size = size; d->idx = 0;
    memset(d->buf, 0, sizeof(float32_t)*size*2);
}

/* Process block in-place (L and R arrays). */
void delay_process_block(delay_t *d, float32_t *L, float32_t *R, uint32_t n, float32_t feedback);

#endif /* DELAY_H */ 