#include "wav_writer.h"
#include "osc.h"
#include <stdlib.h>
#include <stdint.h>

int main(void)
{
    const uint32_t sr = 44100;
    const uint32_t frames = sr; // 1 second
    float *mono = malloc(frames * sizeof(float));
    if (!mono) return 1;

    osc_t osc; osc_reset(&osc);
    osc_sine_block(&osc, mono, frames, 440.0f, (float)sr);

    int16_t *stereo = malloc(frames * 2 * sizeof(int16_t));
    for (uint32_t i = 0; i < frames; ++i) {
        int16_t s = (int16_t)(mono[i] * 32767 * 0.5f); // -6 dBFS
        stereo[2*i] = stereo[2*i+1] = s;
    }

    write_wav("sine.wav", stereo, frames, 2, sr);

    free(mono);
    free(stereo);
    return 0;
} 