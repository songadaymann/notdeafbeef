#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "kick.h"

// Minimal test to reproduce kick_process() parameter corruption
int main(void) {
    printf("🔍 MINIMAL KICK CORRUPTION TEST\n");
    printf("Goal: Isolate if kick_process() corrupts caller's stack/registers\n\n");
    
    // Test data on stack (simulates orchestration context)
    kick_t kick;
    float L[512] = {0};
    float R[512] = {0};
    uint32_t frames = 256;
    
    // Canary values to detect corruption
    void *canary1 = (void*)0xDEADBEEF;
    void *canary2 = (void*)0xCAFEBABE;
    uint32_t canary3 = 0x12345678;
    uint32_t canary4 = 0x87654321;
    
    printf("BEFORE kick_init:\n");
    printf("  kick ptr:    %p\n", &kick);
    printf("  L ptr:       %p\n", L);
    printf("  R ptr:       %p\n", R);
    printf("  frames:      %u\n", frames);
    printf("  canary1:     %p\n", canary1);
    printf("  canary2:     %p\n", canary2);
    printf("  canary3:     0x%x\n", canary3);
    printf("  canary4:     0x%x\n", canary4);
    
    // Initialize kick
    kick_init(&kick, 44100.0f);
    kick_trigger(&kick);
    
    printf("\nAFTER kick_init:\n");
    printf("  canary1:     %p\n", canary1);
    printf("  canary2:     %p\n", canary2);
    printf("  canary3:     0x%x\n", canary3);
    printf("  canary4:     0x%x\n", canary4);
    printf("  kick.pos:    %u\n", kick.pos);
    printf("  kick.len:    %u\n", kick.len);
    
    // Call kick_process (this is where corruption might happen)
    printf("\n🚨 CALLING kick_process() - WATCH FOR CORRUPTION:\n");
    kick_process(&kick, L, R, frames);
    
    printf("\nAFTER kick_process:\n");
    printf("  kick ptr:    %p\n", &kick);
    printf("  L ptr:       %p\n", L);
    printf("  R ptr:       %p\n", R);
    printf("  frames:      %u\n", frames);
    printf("  canary1:     %p %s\n", canary1, (canary1 == (void*)0xDEADBEEF) ? "✅" : "❌ CORRUPTED!");
    printf("  canary2:     %p %s\n", canary2, (canary2 == (void*)0xCAFEBABE) ? "✅" : "❌ CORRUPTED!");
    printf("  canary3:     0x%x %s\n", canary3, (canary3 == 0x12345678) ? "✅" : "❌ CORRUPTED!");
    printf("  canary4:     0x%x %s\n", canary4, (canary4 == 0x87654321) ? "✅" : "❌ CORRUPTED!");
    printf("  kick.pos:    %u\n", kick.pos);
    printf("  L[0]:        %f\n", L[0]);
    printf("  R[0]:        %f\n", R[0]);
    
    // Check if we can make a second call (simulates orchestration sequence)
    printf("\n🔄 SECOND CALL TEST (simulates snare_process call pattern):\n");
    
    void *test_ptr = &kick;
    void *test_L = L;
    void *test_R = R;
    uint32_t test_frames = 128;
    
    printf("  Parameters before second call:\n");
    printf("    kick ptr:    %p\n", test_ptr);
    printf("    L ptr:       %p\n", test_L);
    printf("    R ptr:       %p\n", test_R);
    printf("    frames:      %u\n", test_frames);
    
    // Second call
    kick_process(&kick, L, R, test_frames);
    
    printf("  ✅ Second call completed successfully!\n");
    printf("  Final canary check:\n");
    printf("    canary1:     %p %s\n", canary1, (canary1 == (void*)0xDEADBEEF) ? "✅" : "❌ CORRUPTED!");
    printf("    canary2:     %p %s\n", canary2, (canary2 == (void*)0xCAFEBABE) ? "✅" : "❌ CORRUPTED!");
    printf("    canary3:     0x%x %s\n", canary3, (canary3 == 0x12345678) ? "✅" : "❌ CORRUPTED!");
    printf("    canary4:     0x%x %s\n", canary4, (canary4 == 0x87654321) ? "✅" : "❌ CORRUPTED!");
    
    printf("\n🎯 CONCLUSION: kick_process() in isolation ");
    if (canary1 == (void*)0xDEADBEEF && canary2 == (void*)0xCAFEBABE && 
        canary3 == 0x12345678 && canary4 == 0x87654321) {
        printf("WORKS CORRECTLY ✅\n");
        printf("    Stack corruption happens only in multi-voice orchestration context\n");
    } else {
        printf("CORRUPTS STACK ❌\n");
        printf("    kick_process() has intrinsic stack corruption bug\n");
    }
    
    return 0;
} 