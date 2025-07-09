#include <stdio.h>
#include <stdint.h>

// Minimal kick struct definition (matches kick.h layout)
typedef struct {
    float sr;      // offset 0
    uint32_t pos;  // offset 4  
    uint32_t len;  // offset 8
} kick_t;

// External assembly function declaration
extern void kick_process(kick_t *k, float *L, float *R, uint32_t n);

int main(void) {
    printf("🔍 ULTRA-MINIMAL KICK TEST\n");
    printf("Goal: Isolate exact kick_process crash with minimal dependencies\n\n");
    
    // Minimal setup
    kick_t kick = {
        .sr = 44100.0f,
        .pos = 0,
        .len = 1000  // Small len to limit loop iterations
    };
    
    // Small buffers
    float L[16] = {0.0f};  // Very small to limit crash scope
    float R[16] = {0.0f};
    uint32_t frames = 4;   // Process only 4 samples
    
    printf("Setup:\n");
    printf("  kick.sr:     %f\n", kick.sr);
    printf("  kick.pos:    %u\n", kick.pos);
    printf("  kick.len:    %u\n", kick.len);
    printf("  frames:      %u\n", frames);
    printf("  L buffer:    %p\n", L);
    printf("  R buffer:    %p\n", R);
    
    printf("\n🚨 CALLING kick_process with minimal parameters...\n");
    
    // Direct call to assembly function
    kick_process(&kick, L, R, frames);
    
    printf("✅ SUCCESS: kick_process completed!\n");
    printf("  L[0] = %f\n", L[0]);
    printf("  R[0] = %f\n", R[0]);
    printf("  kick.pos = %u\n", kick.pos);
    
    return 0;
} 