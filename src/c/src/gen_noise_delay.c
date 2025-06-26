#include "wav_writer.h"
#include "osc.h"
#include "noise.h"
#include "delay.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t total_frames = sr * 2; // 2 seconds
    float *L = malloc(sizeof(float)*total_frames);
    float *R = malloc(sizeof(float)*total_frames);
    if(!L||!R) return 1;

    /* First second: 440 Hz sine with 5 ms fade-in/out; then silence */
    const float fade_time = 0.005f;              // 5 ms
    const uint32_t fade_samps = (uint32_t)(sr * fade_time);

    osc_t osc; osc_reset(&osc);
    for(uint32_t i=0;i<total_frames;i++){
        if(i<sr){
            float s; osc_sine_block(&osc,&s,1,440.0f,(float)sr);
            float amp = 0.4f;
            /* linear fade */
            if(i < fade_samps)                amp *= (float)i / fade_samps;
            if(sr - i <= fade_samps)          amp *= (float)(sr - i) / fade_samps;
            L[i]=R[i]=s*amp;
        }else{ L[i]=R[i]=0.0f; }
    }

    /* Prepare delay line of 0.25s */
    uint32_t delay_samples = sr/4;
    float *delay_buf = malloc(sizeof(float)*delay_samples*2);
    delay_t delay; delay_init(&delay,&delay_buf[0],delay_samples);

    printf("Before delay: total_frames=%u delay.size=%u delay.idx=%u\n", total_frames, delay.size, delay.idx);
    delay_process_block(&delay,L,R,total_frames,0.5f);

    /* convert to int16 */
    int16_t *pcm = malloc(sizeof(int16_t)*total_frames*2);
    for(uint32_t i=0;i<total_frames;i++){
        /* clamp to [-1,1] to avoid wrap distortion */
        float vL = L[i]; if(vL>1.0f) vL=1.0f; if(vL<-1.0f) vL=-1.0f;
        float vR = R[i]; if(vR>1.0f) vR=1.0f; if(vR<-1.0f) vR=-1.0f;
        int16_t s = (int16_t)(vL*32767);
        pcm[2*i]=s; pcm[2*i+1]=(int16_t)(vR*32767);
    }

    write_wav("delay.wav",pcm,total_frames,2,sr);

    free(L);free(R);free(delay_buf);free(pcm);
    return 0;
} 