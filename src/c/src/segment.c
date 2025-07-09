#include "wav_writer.h"
#include "generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* extern counter defined in generator_step.c */
extern int g_mid_trigger_count;

#define MAX_SEG_FRAMES 424000 
static float L[MAX_SEG_FRAMES], R[MAX_SEG_FRAMES];
static int16_t pcm[MAX_SEG_FRAMES * 2];

/* Fallback scalar RMS when assembly version not linked */
#ifndef GENERATOR_RMS_ASM_PRESENT
float generator_compute_rms_asm(const float *L, const float *R, uint32_t num_frames)
{
    double sum = 0.0;
    for(uint32_t i=0;i<num_frames;i++){
        double sL = L[i];
        double sR = R[i];
        sum += sL*sL + sR*sR;
    }
    return (float)sqrt(sum / (double)(2*num_frames));
}
#endif

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

    printf("C-DBG before gen_process: step_samples=%u addr=%p\n", g.mt.step_samples, &g.mt.step_samples);
    generator_process(&g, L, R, total_frames);
    
    /* RMS diagnostic to verify audio energy */
    float rms = generator_compute_rms_asm(L, R, total_frames);
    printf("C-POST rms=%f\n", rms);
    printf("DEBUG: MID triggers fired = %d\n", g_mid_trigger_count);

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