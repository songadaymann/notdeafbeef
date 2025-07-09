
#include <stdio.h>
#include <stdint.h>
#include "kick.h"

// Test function to isolate kick_process() stack corruption
int main() {
    kick_t kick;
    float L[512] = {0};  // Test buffers
    float R[512] = {0};
    
    // Initialize kick
    kick_init(&kick, 44100.0f);
    kick_trigger(&kick);
    
    // Save original parameter values
    void *orig_kick_ptr = &kick;
    void *orig_L_ptr = L;
    void *orig_R_ptr = R;
    uint32_t orig_frames = 256;
    
    printf("BEFORE kick_process:\n");
    printf("  kick ptr:    %p\n", orig_kick_ptr);
    printf("  L ptr:       %p\n", orig_L_ptr);
    printf("  R ptr:       %p\n", orig_R_ptr);
    printf("  frames:      %u\n", orig_frames);
    printf("  L[0]:        %f\n", L[0]);
    printf("  R[0]:        %f\n", R[0]);
    printf("  kick.pos:    %u\n", kick.pos);
    
    // Call kick_process (this is where corruption happens in orchestration)
    kick_process(&kick, L, R, orig_frames);
    
    printf("AFTER kick_process:\n");
    printf("  kick ptr:    %p\n", &kick);
    printf("  L ptr:       %p\n", L);
    printf("  R ptr:       %p\n", R);
    printf("  frames:      %u\n", orig_frames);
    printf("  L[0]:        %f\n", L[0]);
    printf("  R[0]:        %f\n", R[0]);
    printf("  kick.pos:    %u\n", kick.pos);
    
    // Test if we can call kick_process again (mimics orchestration pattern)
    printf("\nSecond call test (mimics snare_process call pattern):\n");
    void *test_kick_ptr = &kick;
    void *test_L_ptr = L;
    void *test_R_ptr = R;
    uint32_t test_frames = 256;
    
    printf("  Parameters before second call:\n");
    printf("    kick ptr:    %p\n", test_kick_ptr);
    printf("    L ptr:       %p\n", test_L_ptr);
    printf("    R ptr:       %p\n", test_R_ptr);
    printf("    frames:      %u\n", test_frames);
    
    // This simulates the snare_process call pattern that fails
    kick_process(&kick, L, R, test_frames);
    
    printf("  Second call completed successfully!\n");
    
    return 0;
}
