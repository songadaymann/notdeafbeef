#include "wav_writer.h"
#include "fm_voice.h"
#include "fm_presets.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int main(void)
{
    const uint32_t sr=44100;
    const uint32_t total_frames=sr*2;
    float *L=calloc(total_frames,sizeof(float));
    float *R=calloc(total_frames,sizeof(float));

    fm_voice_t mid; fm_voice_init(&mid,(float)sr);
    fm_voice_t bass; fm_voice_init(&bass,(float)sr);

    fm_params_t presets[4] = {FM_PRESET_BELLS, FM_PRESET_CALM, FM_PRESET_QUANTUM, FM_PRESET_PLUCK};

    for(uint32_t frame=0; frame<total_frames; frame+=256){
        float t=(float)frame/sr;
        if(fmodf(t,0.5f)<1e-4f){
            fm_params_t p = presets[(uint32_t)(t/0.5f)%4];
            fm_voice_trigger(&mid, 880.0f, 0.45f, p.ratio, p.index, p.amp, p.decay);
        }
        if(fmodf(t,1.0f)<1e-4f){
            /* bass fm */
            fm_voice_trigger(&bass, 55.0f, 0.8f, 2.0f, 5.0f, 0.4f, 10.0f);
        }
        uint32_t block = (frame+256<=total_frames)?256:(total_frames-frame);
        fm_voice_process(&mid,&L[frame],&R[frame],block);
        fm_voice_process(&bass,&L[frame],&R[frame],block);
    }

    int16_t *pcm=malloc(sizeof(int16_t)*total_frames*2);
    for(uint32_t i=0;i<total_frames;i++){
        float v=L[i]; if(v>1)v=1;if(v<-1)v=-1;
        int16_t s=(int16_t)(v*32767);
        pcm[2*i]=s; pcm[2*i+1]=s;
    }
    write_wav("fm.wav",pcm,total_frames,2,sr);
    free(L);free(R);free(pcm);
    return 0;
} 