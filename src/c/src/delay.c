#include "delay.h"

#ifndef DELAY_ASM
void delay_process_block(delay_t *d, float32_t *L, float32_t *R, uint32_t n, float32_t feedback)
{
    float32_t *buf = d->buf;
    uint32_t idx = d->idx;
    const uint32_t size = d->size;

    for(uint32_t i=0;i<n;++i){
        // Fetch delayed samples
        float32_t yl = buf[idx*2];
        float32_t yr = buf[idx*2+1];

        // Cache dry samples before we modify the output buffers
        float32_t dryL = L[i];
        float32_t dryR = R[i];

        // Write new values into the delay line (cross-feed with feedback)
        buf[idx*2]   = dryL + yr * feedback;
        buf[idx*2+1] = dryR + yl * feedback;

        // Add delayed signal to the dry signal instead of overwriting it
        L[i] = dryL + yl;
        R[i] = dryR + yr;

        // Advance circular buffer index
        idx++;
        if(idx>=size) idx=0;
    }
    d->idx = idx;
}
#endif // DELAY_ASM 