#include "wav_writer.h"
#include "melody.h"
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t total_frames = sr * 2; // 2 seconds
    float *L = calloc(total_frames, sizeof(float));
    float *R = calloc(total_frames, sizeof(float));

    melody_t mel; melody_init(&mel, (float)sr);

    /* frequencies for simple arpeggio */
    const float freqs[4] = {440.0f, 554.37f, 659.25f, 880.0f};
    uint32_t note_idx = 0;

    for(uint32_t frame=0; frame<total_frames; frame += 256){
        float t = (float)frame / sr;
        /* start new note every 0.5 seconds */
        if (fmodf(t, 0.5f) < 1e-4f){
            melody_trigger(&mel, freqs[note_idx % 4], 0.45f);
            note_idx++;
        }
        uint32_t block = (frame + 256 <= total_frames) ? 256 : (total_frames - frame);
        melody_process(&mel, &L[frame], &R[frame], block);
    }

    int16_t *pcm = malloc(sizeof(int16_t)*total_frames*2);
    for(uint32_t i=0;i<total_frames;i++){
        float v = L[i]; if(v>1) v=1; if(v<-1) v=-1;
        int16_t s = (int16_t)(v*32767);
        pcm[2*i] = s; pcm[2*i+1] = s;
    }

    write_wav("melody.wav", pcm, total_frames, 2, sr);
    free(L); free(R); free(pcm);
    return 0;
} 