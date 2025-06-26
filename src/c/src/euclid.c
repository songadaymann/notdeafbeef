#include "euclid.h"

void euclid_pattern(int pulses, int steps, uint8_t *out)
{
    int bucket = 0;
    for (int i = 0; i < steps; ++i) {
        bucket += pulses;
        if (bucket >= steps) {
            bucket -= steps;
            out[i] = 1;
        } else {
            out[i] = 0;
        }
    }
} 