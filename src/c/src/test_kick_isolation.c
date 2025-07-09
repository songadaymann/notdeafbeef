#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "generator.h"

// Simple test to isolate kick function corruption - Match gen_kick.c pattern
int main(void) {
    printf("Starting kick isolation test...\n");
    
    // Create a simple generator struct
    generator_t g;
    memset(&g, 0, sizeof(g));
    
    // Initialize delay with known values  
    float delay_buf[1000];
    delay_init(&g.delay, delay_buf, 100);
    
    printf("Before kick_process: delay.buf=%p size=%u idx=%u\n", 
           g.delay.buf, g.delay.size, g.delay.idx);
    printf("Delay struct address: %p\n", &g.delay);
    
    // Initialize kick exactly like gen_kick.c
    kick_init(&g.kick, 44100.0f);
    kick_trigger(&g.kick);
    
    // Create small buffers - use smaller blocks like gen_kick.c (256 samples)
    float L[256] = {0};
    float R[256] = {0};
    
    printf("About to call kick_process with 256 samples...\n");
    
    // Call kick_process with smaller block
    kick_process(&g.kick, L, R, 256);
    
    printf("After kick_process: delay.buf=%p size=%u idx=%u\n", 
           g.delay.buf, g.delay.size, g.delay.idx);
    
    printf("Test completed successfully!\n");
    return 0;
} 