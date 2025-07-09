#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <arm_neon.h>
#include "./src/c/include/generator.h"

int main() {
    printf("offsetof(generator_t, mt.step_samples) = %zu\n", offsetof(generator_t, mt.step_samples));
    printf("offsetof(generator_t, event_idx) = %zu\n", offsetof(generator_t, event_idx));
    printf("offsetof(generator_t, step) = %zu\n", offsetof(generator_t, step));
    printf("offsetof(generator_t, pos_in_step) = %zu\n", offsetof(generator_t, pos_in_step));
    printf("offsetof(generator_t, delay) = %zu\n", offsetof(generator_t, delay));
    printf("offsetof(generator_t, limiter) = %zu\n", offsetof(generator_t, limiter));
    printf("offsetof(generator_t, mel) = %zu\n", offsetof(generator_t, mel));
    return 0;
}
