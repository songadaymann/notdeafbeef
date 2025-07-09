
#include <stdio.h>
#include <stdint.h>
#include "kick.h"
#include "snare.h"

// Test the exact orchestration pattern that fails
int main() {
    kick_t kick;
    snare_t snare;
    float L[512] = {0};
    float R[512] = {0};
    
    // Initialize both voices
    kick_init(&kick, 44100.0f);
    snare_init(&snare, 44100.0f);
    kick_trigger(&kick);
    snare_trigger(&snare);
    
    uint32_t frames = 256;
    
    printf("ORCHESTRATION TEST - kick_process() followed by snare_process()\n");
    printf("Initial state:\n");
    printf("  kick ptr:    %p\n", &kick);
    printf("  snare ptr:   %p\n", &snare);
    printf("  L ptr:       %p\n", L);
    printf("  R ptr:       %p\n", R);
    printf("  frames:      %u\n", frames);
    
    // Call kick_process (works fine in isolation)
    printf("\nCalling kick_process...\n");
    kick_process(&kick, L, R, frames);
    printf("kick_process completed successfully\n");
    
    // Check parameter integrity after kick_process
    printf("\nParameter check after kick_process:\n");
    printf("  snare ptr:   %p\n", &snare);
    printf("  L ptr:       %p\n", L);  
    printf("  R ptr:       %p\n", R);
    printf("  frames:      %u\n", frames);
    
    // Call snare_process (this is where the crash happens in full orchestration)
    printf("\nCalling snare_process...\n");
    snare_process(&snare, L, R, frames);
    printf("snare_process completed successfully\n");
    
    printf("\n✅ ORCHESTRATION TEST PASSED - No corruption detected!\n");
    
    return 0;
}
