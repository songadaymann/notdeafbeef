#!/usr/bin/env python3
"""
Runtime Integration Debugging for Multi-Voice Orchestration

Focused on isolating the exact runtime segfault in multi-voice assembly integration.
Individual voice functions work perfectly, but orchestration crashes.

Strategy: Incremental voice activation with LLDB crash analysis.
"""

import subprocess
import time
from pathlib import Path

class RuntimeIntegrationDebugger:
    """Systematic runtime debugging for multi-voice assembly integration"""
    
    def __init__(self, c_dir: Path):
        self.c_dir = Path(c_dir)
        
    def test_incremental_voice_activation(self):
        """Test activating one voice at a time to isolate the problematic interaction"""
        print("🎵 === INCREMENTAL VOICE ACTIVATION TESTING ===")
        print("Strategy: Enable assembly voices one at a time to isolate segfault source")
        print()
        
        # Test configurations with different voice combinations
        voice_configs = [
            ("no_voice_asm", ["-DOSC_SINE_ASM", "-DOSC_SHAPES_ASM"]),  # Core only
            ("kick_only", ["-DOSC_SINE_ASM", "-DOSC_SHAPES_ASM", "-DKICK_ASM"]),
            ("kick_snare", ["-DOSC_SINE_ASM", "-DOSC_SHAPES_ASM", "-DKICK_ASM", "-DSNARE_ASM"]),
            ("kick_snare_hat", ["-DOSC_SINE_ASM", "-DOSC_SHAPES_ASM", "-DKICK_ASM", "-DSNARE_ASM", "-DHAT_ASM"]),
            ("all_voices", ["-DOSC_SINE_ASM", "-DOSC_SHAPES_ASM", "-DKICK_ASM", "-DSNARE_ASM", "-DHAT_ASM", "-DMELODY_ASM"]),
        ]
        
        results = {}
        
        for config_name, cflags in voice_configs:
            print(f"\n🔧 Testing Configuration: {config_name}")
            print(f"   Flags: {' '.join(cflags)}")
            
            # Build with specific configuration
            build_result = self.build_segment_with_flags(config_name, cflags)
            
            if build_result["success"]:
                # Test runtime execution
                runtime_result = self.test_segment_execution(config_name)
                results[config_name] = {
                    "build": build_result,
                    "runtime": runtime_result
                }
                
                if runtime_result["success"]:
                    print(f"   ✅ RUNTIME SUCCESS: Configuration works!")
                else:
                    print(f"   ❌ RUNTIME FAILURE: Segfault detected")
                    print(f"   📍 This configuration introduces the segfault")
                    break  # Stop at first failing configuration to isolate
            else:
                print(f"   ❌ BUILD FAILURE: Cannot test runtime")
                results[config_name] = {"build": build_result, "runtime": None}
        
        return results
    
    def build_segment_with_flags(self, config_name: str, cflags: list) -> dict:
        """Build segment with specific compiler flags"""
        env_cflags = " ".join(cflags)
        
        # Clean and build with custom flags
        cmd = f'make clean && CFLAGS="-std=c11 -Wall -Wextra -O2 -Iinclude $(SDL_CFLAGS) -Dfloat32_t=float {env_cflags}" make segment USE_ASM=1'
        
        result = subprocess.run(
            cmd,
            shell=True,
            cwd=self.c_dir,
            capture_output=True,
            text=True
        )
        
        return {
            "success": result.returncode == 0,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "config": config_name,
            "flags": cflags
        }
    
    def test_segment_execution(self, config_name: str) -> dict:
        """Test segment execution and capture any segfaults"""
        print(f"   🏃 Running segment execution test...")
        
        # Run with timeout to prevent hanging
        try:
            result = subprocess.run(
                ["./bin/segment", "0xTEST123"],
                cwd=self.c_dir,
                capture_output=True,
                text=True,
                timeout=10  # 10 second timeout
            )
            
            success = result.returncode == 0
            if success:
                print(f"   ✅ Execution completed successfully")
            else:
                print(f"   ❌ Execution failed with code {result.returncode}")
                if result.returncode == -11:  # SIGSEGV
                    print(f"   💥 Segmentation fault detected!")
            
            return {
                "success": success,
                "returncode": result.returncode,
                "stdout": result.stdout,
                "stderr": result.stderr,
                "config": config_name
            }
            
        except subprocess.TimeoutExpired:
            print(f"   ⏰ Execution timeout (likely infinite loop)")
            return {
                "success": False,
                "returncode": "timeout",
                "stdout": "",
                "stderr": "Process timed out",
                "config": config_name
            }
    
    def analyze_lldb_crash_details(self, failing_config: str):
        """Use LLDB to get detailed crash analysis for the failing configuration"""
        print(f"\n🔍 === LLDB CRASH ANALYSIS: {failing_config} ===")
        
        lldb_script = f"""
#!/bin/bash
lldb ./bin/segment << 'EOF'
# Set debug flags for comprehensive info
settings set target.process.stop-on-sharedlibrary-events false

# Run with crash detection
run 0xDEBUG123

# When crash occurs, show detailed info
bt
register read
frame info
memory read --format hex --size 64 $rsp
memory read --format hex --size 64 $rbp

# Show assembly context
disassemble --pc
disassemble --frame

quit
EOF
"""
        
        # Write LLDB script
        lldb_script_path = self.c_dir / f"debug_{failing_config}.sh"
        with open(lldb_script_path, 'w') as f:
            f.write(lldb_script)
        
        # Make executable and run
        subprocess.run(["chmod", "+x", str(lldb_script_path)])
        
        print(f"   📋 LLDB script created: {lldb_script_path}")
        print(f"   💡 Run manually: cd {self.c_dir} && ./debug_{failing_config}.sh")
        
        return lldb_script_path
    
    def run_runtime_debug_session(self):
        """Execute complete runtime debugging session"""
        print("🚀 === RUNTIME INTEGRATION DEBUG SESSION ===")
        print("Focus: Multi-voice assembly orchestration segfaults")
        print("Goal: Isolate which voice combination triggers the crash")
        print()
        
        # Phase 1: Incremental voice activation
        voice_results = self.test_incremental_voice_activation()
        
        # Phase 2: Identify first failing configuration
        first_failure = None
        for config_name, result in voice_results.items():
            if result and result.get("runtime") and not result["runtime"]["success"]:
                first_failure = config_name
                break
        
        if first_failure:
            print(f"\n🎯 === ISOLATED PROBLEMATIC CONFIGURATION ===")
            print(f"Configuration: {first_failure}")
            print(f"This is the minimal set of voice functions that triggers the segfault")
            
            # Phase 3: Generate LLDB debugging script for detailed analysis
            lldb_script = self.analyze_lldb_crash_details(first_failure)
            
            print(f"\n📋 === DEBUGGING RECOMMENDATIONS ===")
            print(f"1. Run LLDB analysis: cd {self.c_dir} && ./debug_{first_failure}.sh")
            print(f"2. Focus debugging efforts on voice functions in {first_failure} configuration")
            print(f"3. Compare stack frame usage between working individual tests and failing orchestration")
            print(f"4. Check for register corruption in voice function call sequences")
            
        else:
            print(f"\n🤔 === UNEXPECTED RESULT ===")
            print(f"All tested configurations succeeded - the segfault might be intermittent")
            print(f"or dependent on specific input values/timing")
        
        return {
            "voice_results": voice_results,
            "first_failure": first_failure,
            "recommended_focus": first_failure
        }


def main():
    """Execute runtime debugging session"""
    script_root = Path(__file__).parent
    project_root = script_root.parent
    c_dir = project_root / "src" / "c"
    
    if not c_dir.exists():
        print(f"❌ C source directory not found: {c_dir}")
        return
    
    debugger = RuntimeIntegrationDebugger(c_dir)
    results = debugger.run_runtime_debug_session()
    
    print(f"\n💾 Next steps saved for manual execution")
    print(f"🎯 Focus debugging efforts on: {results.get('first_failure', 'all configurations')}")


if __name__ == "__main__":
    main() 