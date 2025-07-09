#!/usr/bin/env python3
"""
Stack Corruption Diagnostic for kick_process() Assembly Function

The LLDB analysis shows that kick_process() corrupts caller parameters,
causing snare_process() to receive garbage values. This tool creates
isolated tests to identify the exact corruption mechanism.
"""

import subprocess
from pathlib import Path

def create_kick_isolation_test():
    """Create a minimal test that calls kick_process() in isolation"""
    
    test_code = '''
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
    
    printf("BEFORE kick_process:\\n");
    printf("  kick ptr:    %p\\n", orig_kick_ptr);
    printf("  L ptr:       %p\\n", orig_L_ptr);
    printf("  R ptr:       %p\\n", orig_R_ptr);
    printf("  frames:      %u\\n", orig_frames);
    printf("  L[0]:        %f\\n", L[0]);
    printf("  R[0]:        %f\\n", R[0]);
    printf("  kick.pos:    %u\\n", kick.pos);
    
    // Call kick_process (this is where corruption happens in orchestration)
    kick_process(&kick, L, R, orig_frames);
    
    printf("AFTER kick_process:\\n");
    printf("  kick ptr:    %p\\n", &kick);
    printf("  L ptr:       %p\\n", L);
    printf("  R ptr:       %p\\n", R);
    printf("  frames:      %u\\n", orig_frames);
    printf("  L[0]:        %f\\n", L[0]);
    printf("  R[0]:        %f\\n", R[0]);
    printf("  kick.pos:    %u\\n", kick.pos);
    
    // Test if we can call kick_process again (mimics orchestration pattern)
    printf("\\nSecond call test (mimics snare_process call pattern):\\n");
    void *test_kick_ptr = &kick;
    void *test_L_ptr = L;
    void *test_R_ptr = R;
    uint32_t test_frames = 256;
    
    printf("  Parameters before second call:\\n");
    printf("    kick ptr:    %p\\n", test_kick_ptr);
    printf("    L ptr:       %p\\n", test_L_ptr);
    printf("    R ptr:       %p\\n", test_R_ptr);
    printf("    frames:      %u\\n", test_frames);
    
    // This simulates the snare_process call pattern that fails
    kick_process(&kick, L, R, test_frames);
    
    printf("  Second call completed successfully!\\n");
    
    return 0;
}
'''
    
    return test_code

def create_dual_voice_test():
    """Create a test that calls kick_process followed by snare_process like in orchestration"""
    
    test_code = '''
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
    
    printf("ORCHESTRATION TEST - kick_process() followed by snare_process()\\n");
    printf("Initial state:\\n");
    printf("  kick ptr:    %p\\n", &kick);
    printf("  snare ptr:   %p\\n", &snare);
    printf("  L ptr:       %p\\n", L);
    printf("  R ptr:       %p\\n", R);
    printf("  frames:      %u\\n", frames);
    
    // Call kick_process (works fine in isolation)
    printf("\\nCalling kick_process...\\n");
    kick_process(&kick, L, R, frames);
    printf("kick_process completed successfully\\n");
    
    // Check parameter integrity after kick_process
    printf("\\nParameter check after kick_process:\\n");
    printf("  snare ptr:   %p\\n", &snare);
    printf("  L ptr:       %p\\n", L);  
    printf("  R ptr:       %p\\n", R);
    printf("  frames:      %u\\n", frames);
    
    // Call snare_process (this is where the crash happens in full orchestration)
    printf("\\nCalling snare_process...\\n");
    snare_process(&snare, L, R, frames);
    printf("snare_process completed successfully\\n");
    
    printf("\\n✅ ORCHESTRATION TEST PASSED - No corruption detected!\\n");
    
    return 0;
}
'''
    
    return test_code

def compile_and_run_test(test_name: str, test_code: str, c_dir: Path):
    """Compile and run a diagnostic test"""
    
    test_file = c_dir / f"{test_name}.c"
    test_binary = c_dir / f"{test_name}"
    
    # Write test code
    with open(test_file, 'w') as f:
        f.write(test_code)
    
    # Compile with assembly version
    cmd = f"clang -std=c11 -Wall -Wextra -O0 -g -Iinclude -DKICK_ASM -DSNARE_ASM -DOSC_SINE_ASM -DOSC_SHAPES_ASM -o {test_name} {test_name}.c src/kick.o src/snare.o ../asm/active/kick.o ../asm/active/snare.o ../asm/active/osc_sine.o ../asm/active/osc_shapes.o"
    
    print(f"🔧 Compiling {test_name}...")
    build_result = subprocess.run(cmd, shell=True, cwd=c_dir, capture_output=True, text=True)
    
    if build_result.returncode != 0:
        print(f"❌ Build failed for {test_name}")
        print(f"Error: {build_result.stderr}")
        return None
    
    print(f"✅ Build successful for {test_name}")
    
    # Run test
    print(f"🏃 Running {test_name}...")
    run_result = subprocess.run([f"./{test_name}"], cwd=c_dir, capture_output=True, text=True)
    
    if run_result.returncode != 0:
        print(f"❌ Runtime failure for {test_name}")
        print(f"Exit code: {run_result.returncode}")
        print(f"Stdout: {run_result.stdout}")
        print(f"Stderr: {run_result.stderr}")
        return "FAILED"
    else:
        print(f"✅ Runtime success for {test_name}")
        print("Output:")
        print(run_result.stdout)
        return "PASSED"

def main():
    """Run stack corruption diagnostics"""
    script_root = Path(__file__).parent
    project_root = script_root.parent
    c_dir = project_root / "src" / "c"
    
    print("🔍 === STACK CORRUPTION DIAGNOSTIC ===")
    print("Goal: Isolate kick_process() parameter corruption mechanism")
    print()
    
    # Ensure we have built objects
    print("🏗️ Building required objects...")
    build_cmd = "make clean && make src/kick.o src/snare.o USE_ASM=1"
    subprocess.run(build_cmd, shell=True, cwd=c_dir)
    
    # Test 1: Kick in isolation
    print("\n📋 Test 1: kick_process() in isolation")
    test1_code = create_kick_isolation_test()
    test1_result = compile_and_run_test("test_kick_isolation", test1_code, c_dir)
    
    # Test 2: Dual voice orchestration pattern
    print("\n📋 Test 2: kick_process() + snare_process() orchestration")
    test2_code = create_dual_voice_test()
    test2_result = compile_and_run_test("test_dual_voice", test2_code, c_dir)
    
    # Analysis
    print("\n📊 === DIAGNOSTIC RESULTS ===")
    print(f"Kick isolation test:     {test1_result}")
    print(f"Dual voice orchestration: {test2_result}")
    
    if test1_result == "PASSED" and test2_result == "FAILED":
        print("\n🎯 CONCLUSION: Issue occurs only in multi-voice orchestration context")
        print("   Suggests interaction between multiple assembly voice functions")
    elif test1_result == "FAILED":
        print("\n🎯 CONCLUSION: kick_process() has intrinsic parameter corruption")
        print("   Focus debugging on kick.s stack frame and register preservation")
    elif test1_result == "PASSED" and test2_result == "PASSED":
        print("\n🤔 CONCLUSION: Corruption might be context-dependent")
        print("   May require full generator context to reproduce")

if __name__ == "__main__":
    main() 