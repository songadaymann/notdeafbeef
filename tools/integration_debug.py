#!/usr/bin/env python3
"""
Integration Debugging Framework for NotDeafbeef Audio Engine

This tool provides systematic approaches to debug remaining integration issues
after the successful resolution of voice assembly stack frame corruption.

Based on roadmap analysis: Individual voice functions work perfectly, but
full segment generation has build system conflicts and multi-voice interaction issues.
"""

import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class IntegrationDebugger:
    """Systematic debugging framework for audio engine integration issues"""
    
    def __init__(self, c_dir: Path):
        self.c_dir = Path(c_dir)
        self.results = {}
        
    def run_build_test(self, test_name: str, make_args: List[str]) -> Dict:
        """Run a build test and capture results"""
        print(f"\n🔧 Testing: {test_name}")
        print(f"   Command: make {' '.join(make_args)}")
        
        start_time = time.time()
        result = subprocess.run(
            ["make"] + make_args,
            cwd=self.c_dir,
            capture_output=True,
            text=True
        )
        duration = time.time() - start_time
        
        test_result = {
            "name": test_name,
            "success": result.returncode == 0,
            "duration": duration,
            "stdout": result.stdout,
            "stderr": result.stderr,
            "args": make_args
        }
        
        if test_result["success"]:
            print(f"   ✅ SUCCESS ({duration:.2f}s)")
        else:
            print(f"   ❌ FAILED ({duration:.2f}s)")
            # Extract key error info
            if "duplicate symbol" in result.stderr:
                print(f"   📍 Duplicate symbol conflict detected")
            elif "Undefined symbols" in result.stderr:
                print(f"   📍 Missing symbols detected")
            elif "architecture" in result.stderr:
                print(f"   📍 Architecture mismatch detected")
        
        self.results[test_name] = test_result
        return test_result
    
    def test_clean_build_matrix(self) -> Dict[str, Dict]:
        """Test clean builds across different configurations"""
        print("🏗️ === BUILD MATRIX TESTING ===")
        
        tests = [
            ("pure_c_segment", ["clean", "segment", "USE_ASM=0"]),
            ("pure_c_individual_kick", ["clean", "kick", "USE_ASM=0"]),
            ("asm_individual_kick", ["clean", "kick", "USE_ASM=1"]),
            ("asm_individual_snare", ["clean", "snare", "USE_ASM=1"]),
            ("asm_individual_hat", ["clean", "hat", "USE_ASM=1"]),
            ("asm_individual_melody", ["clean", "melody", "USE_ASM=1"]),
            ("asm_segment_full", ["clean", "segment", "USE_ASM=1"]),
        ]
        
        matrix_results = {}
        for test_name, args in tests:
            matrix_results[test_name] = self.run_build_test(test_name, args)
            
        return matrix_results
    
    def test_incremental_assembly_integration(self) -> Dict[str, Dict]:
        """Test incremental assembly integration to isolate conflicts"""
        print("\n🔄 === INCREMENTAL ASSEMBLY INTEGRATION ===")
        
        # Start with core assembly only, gradually add voice functions
        incremental_tests = [
            ("core_asm_only", ["clean", "segment", "USE_ASM=1", "VOICE_ASM=0"]),
            ("core_plus_kick", ["clean", "segment", "USE_ASM=1", "KICK_ONLY=1"]),
            ("core_plus_snare", ["clean", "segment", "USE_ASM=1", "SNARE_ONLY=1"]),
            ("core_plus_hat", ["clean", "segment", "USE_ASM=1", "HAT_ONLY=1"]),
            ("core_plus_melody", ["clean", "segment", "USE_ASM=1", "MELODY_ONLY=1"]),
        ]
        
        incremental_results = {}
        for test_name, args in incremental_tests:
            incremental_results[test_name] = self.run_build_test(test_name, args)
            
        return incremental_results
    
    def analyze_symbol_conflicts(self) -> Dict[str, List[str]]:
        """Analyze duplicate symbol and undefined symbol issues"""
        print("\n🔍 === SYMBOL CONFLICT ANALYSIS ===")
        
        conflicts = {
            "duplicate_symbols": [],
            "undefined_symbols": [],
            "architecture_issues": []
        }
        
        for test_name, result in self.results.items():
            if not result["success"]:
                stderr = result["stderr"]
                
                # Extract duplicate symbols
                if "duplicate symbol" in stderr:
                    lines = stderr.split('\n')
                    for line in lines:
                        if "duplicate symbol" in line:
                            symbol = line.split("'")[1] if "'" in line else "unknown"
                            conflicts["duplicate_symbols"].append(f"{test_name}: {symbol}")
                
                # Extract undefined symbols
                if "Undefined symbols" in stderr:
                    lines = stderr.split('\n')
                    capturing = False
                    for line in lines:
                        if "Undefined symbols for architecture" in line:
                            capturing = True
                        elif capturing and line.strip().startswith('"'):
                            symbol = line.strip().split('"')[1]
                            conflicts["undefined_symbols"].append(f"{test_name}: {symbol}")
                        elif capturing and line.strip() == "":
                            capturing = False
                
                # Extract architecture issues
                if "architecture" in stderr and "ignoring file" in stderr:
                    lines = stderr.split('\n')
                    for line in lines:
                        if "ignoring file" in line and "architecture" in line:
                            conflicts["architecture_issues"].append(f"{test_name}: {line.strip()}")
        
        return conflicts
    
    def generate_debug_makefile_patches(self, conflicts: Dict) -> List[str]:
        """Generate suggested Makefile patches based on conflict analysis"""
        print("\n🛠️ === MAKEFILE PATCH SUGGESTIONS ===")
        
        patches = []
        
        if conflicts["duplicate_symbols"]:
            patches.append("""
# Fix for duplicate symbols: Conditional object inclusion
ifeq ($(USE_ASM),1)
  # Assembly mode: exclude C implementations of assembly functions
  KICK_OBJ := # Skip src/kick.o when using kick.s
  SNARE_OBJ := # Skip src/snare.o when using snare.s  
  HAT_OBJ := # Skip src/hat.o when using hat.s
  MELODY_OBJ := # Skip src/melody.o when using melody.s
else
  # C mode: include all C objects
  KICK_OBJ := src/kick.o
  SNARE_OBJ := src/snare.o
  HAT_OBJ := src/hat.o
  MELODY_OBJ := src/melody.o
endif
""")
        
        if conflicts["undefined_symbols"]:
            patches.append("""
# Fix for undefined symbols: Ensure all dependencies are included
GEN_OBJ := $(ASM_OBJ) $(KICK_OBJ) $(SNARE_OBJ) $(HAT_OBJ) $(MELODY_OBJ) \\
           src/generator.o src/fm_voice.o src/fm_presets.o src/event_queue.o \\
           src/simple_voice.o src/delay.o src/wav_writer.o
""")
        
        if conflicts["architecture_issues"]:
            patches.append("""
# Fix for architecture issues: Force clean rebuild for architecture changes
.PHONY: force-clean-rebuild
force-clean-rebuild:
	rm -f src/*.o ../asm/active/*.o
	$(MAKE) all
""")
        
        return patches
    
    def run_comprehensive_debug_session(self) -> Dict:
        """Run complete debugging session with all strategies"""
        print("🚀 === COMPREHENSIVE INTEGRATION DEBUG SESSION ===")
        print("   Systematic analysis of remaining integration challenges")
        print("   after successful voice assembly stack frame corruption fix")
        print()
        
        # Phase 1: Build matrix
        matrix_results = self.test_clean_build_matrix()
        
        # Phase 2: Incremental integration  
        incremental_results = self.test_incremental_assembly_integration()
        
        # Phase 3: Conflict analysis
        conflicts = self.analyze_symbol_conflicts()
        
        # Phase 4: Generate fixes
        patches = self.generate_debug_makefile_patches(conflicts)
        
        # Summary report
        summary = {
            "total_tests": len(self.results),
            "successful_tests": len([r for r in self.results.values() if r["success"]]),
            "failed_tests": len([r for r in self.results.values() if not r["success"]]),
            "primary_issues": conflicts,
            "suggested_patches": patches
        }
        
        self.print_final_report(summary)
        return summary
    
    def print_final_report(self, summary: Dict):
        """Print comprehensive final debugging report"""
        print("\n📊 === FINAL INTEGRATION DEBUG REPORT ===")
        print(f"Tests run: {summary['total_tests']}")
        print(f"✅ Successful: {summary['successful_tests']}")  
        print(f"❌ Failed: {summary['failed_tests']}")
        print()
        
        if summary['primary_issues']['duplicate_symbols']:
            print("🔧 DUPLICATE SYMBOL CONFLICTS:")
            for issue in summary['primary_issues']['duplicate_symbols']:
                print(f"   • {issue}")
        
        if summary['primary_issues']['undefined_symbols']:
            print("\n🔗 UNDEFINED SYMBOL ISSUES:")
            for issue in summary['primary_issues']['undefined_symbols']:
                print(f"   • {issue}")
        
        if summary['primary_issues']['architecture_issues']:
            print("\n🏗️ ARCHITECTURE MISMATCH ISSUES:")
            for issue in summary['primary_issues']['architecture_issues']:
                print(f"   • {issue}")
        
        print("\n🎯 NEXT STEPS FOR PURE ASSEMBLY COMPLETION:")
        print("   1. Apply Makefile patches to fix build system conflicts")
        print("   2. Test incremental assembly integration")
        print("   3. Debug any remaining multi-voice interaction issues")
        print("   4. Achieve pure assembly segment.wav generation! 🏆")


def main():
    """Main debugging session"""
    script_root = Path(__file__).parent
    project_root = script_root.parent
    c_dir = project_root / "src" / "c"
    
    if not c_dir.exists():
        print(f"❌ C source directory not found: {c_dir}")
        return
    
    debugger = IntegrationDebugger(c_dir)
    results = debugger.run_comprehensive_debug_session()
    
    # Save results for future analysis
    results_file = script_root / "integration_debug_results.json"
    with open(results_file, 'w') as f:
        # Convert results to JSON-serializable format
        json_results = {
            name: {k: v for k, v in result.items() if k != 'stdout' and k != 'stderr'}
            for name, result in debugger.results.items()
        }
        json.dump(json_results, f, indent=2)
    
    print(f"\n💾 Detailed results saved to: {results_file}")


if __name__ == "__main__":
    main() 