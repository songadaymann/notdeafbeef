#include "wav_writer.h"
#include "hat.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t total_frames = sr * 2;
    float *L = calloc(total_frames, sizeof(float));
    float *R = calloc(total_frames, sizeof(float));

    hat_t h; hat_init(&h, (float)sr, 0x12345678);

    /* trigger every 0.25s */
    for(uint32_t frame=0; frame< total_frames; frame += 128){
        float t = (float)frame / sr;
        if (fabsf(fmodf(t,0.25f)) < 1e-4f) hat_trigger(&h);
        uint32_t block = (frame+128<=total_frames)?128:(total_frames-frame);
        hat_process(&h, &L[frame], &R[frame], block);
    }

    int16_t *pcm = malloc(sizeof(int16_t)*total_frames*2);
    for(uint32_t i=0;i<total_frames;i++){
        float v=L[i]; if(v>1)v=1;if(v<-1)v=-1;
        pcm[2*i]= (int16_t)(v*32767);
        pcm[2*i+1]=pcm[2*i];
    }
    write_wav("hat.wav", pcm, total_frames,2,sr);
    free(L);free(R);free(pcm);
    return 0;
} 