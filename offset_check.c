#include <stdio.h>
#include <stddef.h>
#define float32_t float
#include "src/c/include/generator.h"
int main(){
    printf("offset step_samples %zu\n", offsetof(generator_t, mt.step_samples));
    printf("offset pos_in_step %zu\n", offsetof(generator_t, pos_in_step));
    printf("offset step %zu\n", offsetof(generator_t, step));
    printf("offset event_idx %zu\n", offsetof(generator_t, event_idx));
    printf("offset delay %zu\n", offsetof(generator_t, delay));
    printf("offset limiter %zu\n", offsetof(generator_t, limiter));
    return 0;
}
