#include "wav_writer.h"
#include "fm_voice.h"
#include "fm_presets.h"
#include <stdlib.h>
#include <stdint.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t frames = sr * 2; // 2 seconds
    float *L = calloc(frames, sizeof(float));
    float *R = calloc(frames, sizeof(float));

    fm_voice_t bass; fm_voice_init(&bass, (float)sr);
    fm_params_t p = FM_BASS_DEFAULT; p.amp *= 2.0f; // louder for debug
    fm_voice_trigger(&bass, 110.0f, 1.5f, p.ratio, p.index, p.amp, p.decay);

    fm_voice_process(&bass, L, R, frames);

    int16_t *pcm = malloc(sizeof(int16_t)*frames*2);
    for(uint32_t i=0;i<frames;i++){
        float v=L[i]; if(v>1)v=1; if(v<-1)v=-1;
        int16_t s = (int16_t)(v*32767);
        pcm[2*i]=s; pcm[2*i+1]=s;
    }
    write_wav("bass_only.wav", pcm, frames, 2, sr);
    free(L); free(R); free(pcm);
    return 0;
} 