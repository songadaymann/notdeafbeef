#include "wav_writer.h"
#include "kick.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t total_frames = sr * 2; // 2 seconds
    float *L = calloc(total_frames, sizeof(float));
    float *R = calloc(total_frames, sizeof(float));

    kick_t kick; kick_init(&kick, (float)sr);

    printf("L base: %p\n", (void*)L);

    /* Trigger kick at 0.0, 0.5, 1.0, 1.5 sec */
    for(uint32_t frame = 0; frame < total_frames; frame += 256){
        float t = (float)frame / sr;
        if (fabsf(fmodf(t, 0.5f)) < 1e-4f) {
            kick_trigger(&kick);
        }
        printf("call @ frame %u, ptr=%p\n", frame, (void*)&L[frame]);
        uint32_t block = (frame + 256 <= total_frames) ? 256 : (total_frames - frame);
        kick_process(&kick, &L[frame], &R[frame], block);
    }

    /* convert to int16 wav */
    int16_t *pcm = malloc(sizeof(int16_t)*total_frames*2);
    for(uint32_t i=0;i<total_frames;i++){
        float vL=L[i]; if(vL>1) vL=1; if(vL<-1) vL=-1;
        pcm[2*i] = (int16_t)(vL*32767);
        pcm[2*i+1] = pcm[2*i];
    }
    write_wav("kick.wav", pcm, total_frames, 2, sr);

    free(L);free(R);free(pcm);
    return 0;
} 