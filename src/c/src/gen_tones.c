#include "wav_writer.h"
#include "osc.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

static void write_wave(const char *name, void (*render)(osc_t*,float*,uint32_t,float,float))
{
    const uint32_t sr = 44100;
    const uint32_t frames = sr; // 1 s
    float *mono = malloc(frames * sizeof(float));
    int16_t *st = malloc(frames * 2 * sizeof(int16_t));
    if (!mono || !st) { exit(1);}    

    osc_t o; osc_reset(&o);
    render(&o, mono, frames, 440.0f, (float)sr);
    for (uint32_t i=0;i<frames;++i){
        int16_t s = (int16_t)(mono[i]*32767*0.5f);
        st[2*i]=st[2*i+1]=s;
    }
    write_wav(name, st, frames, 2, sr);

    free(mono); free(st);
}

int main(void)
{
    write_wave("saw.wav", osc_saw_block);
    write_wave("square.wav", osc_square_block);
    write_wave("triangle.wav", osc_triangle_block);
    return 0;
} 