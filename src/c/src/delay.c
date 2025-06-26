#include "delay.h"

void delay_process_block(delay_t *d, float32_t *L, float32_t *R, uint32_t n, float32_t feedback)
{
    float32_t *buf = d->buf;
    uint32_t idx = d->idx;
    const uint32_t size = d->size;

    for(uint32_t i=0;i<n;++i){
        float32_t yl = buf[idx*2];
        float32_t yr = buf[idx*2+1];
        buf[idx*2]   = L[i] + yr * feedback;
        buf[idx*2+1] = R[i] + yl * feedback;
        L[i] = yl;
        R[i] = yr;
        idx++;
        if(idx>=size) idx=0;
    }
    d->idx = idx;
} 