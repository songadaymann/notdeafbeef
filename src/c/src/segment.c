#include "wav_writer.h"
#include "generator.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SEG_FRAMES 424000 
static float L[MAX_SEG_FRAMES], R[MAX_SEG_FRAMES];
static int16_t pcm[MAX_SEG_FRAMES * 2];

int main(int argc, char **argv)
{
    uint64_t seed = 0xCAFEBABEULL;
    if(argc > 1) {
        seed = strtoull(argv[1], NULL, 0);
    }
    
    generator_t g;
    generator_init(&g, seed);

    uint32_t total_frames = g.mt.seg_frames;
    if(total_frames > MAX_SEG_FRAMES) total_frames = MAX_SEG_FRAMES;

    generator_process(&g, L, R, total_frames);
    
    for(uint32_t i=0;i<total_frames;i++){
        pcm[2*i]   = (int16_t)(L[i]*32767);
        pcm[2*i+1] = (int16_t)(R[i]*32767);
    }

    char wavname[64];
    sprintf(wavname, "seed_0x%llx.wav", (unsigned long long)seed);
    write_wav(wavname, pcm, total_frames, 2, SR);
    printf("Wrote %s (%u frames, %.2f bpm, root %.2f Hz)\n", wavname, total_frames, g.mt.bpm, g.music.root_freq);

    return 0;
} 