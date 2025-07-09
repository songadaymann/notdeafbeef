#!/usr/bin/env python3
"""
Comprehensive WAV Test Generator for NotDeafbeef
Generates WAV files for all individual sounds in both C and ASM implementations.
"""

import subprocess
import shutil
import pathlib
import os
import hashlib
from pathlib import Path

# All available individual sound generators from the Makefile
AUDIO_TARGETS = [
    # Basic oscillators
    ("sine", "sine.wav"),
    ("tones", ["saw.wav", "square.wav", "triangle.wav"]),  # tones generates multiple files
    
    # Effects
    ("delay", "delay.wav"),
    
    # Percussion
    ("kick", "kick.wav"),
    ("snare", "snare.wav"), 
    ("hat", "hat.wav"),
    
    # Melodic
    ("melody", "melody.wav"),
    
    # FM Synthesis  
    ("fm", "fm.wav"),
    ("bells", "bells-c.wav"),
    ("calm", "calm-c.wav"),
    ("quantum", "quantum-c.wav"),
    ("pluck", "pluck-c.wav"),
    
    # Bass
    ("bass", "bass_only.wav"),
    ("bass_quantum", "bass_quantum.wav"),
    ("bass_plucky", "bass_plucky.wav"),
]

def run_make_command(c_dir, target, use_asm=False):
    """Run a make command with proper environment setup"""
    env = os.environ.copy()
    env['USE_ASM'] = '1' if use_asm else '0'
    
    cmd = ["make", "-C", str(c_dir), "clean", target]
    result = subprocess.run(cmd, env=env, capture_output=True, text=True)
    return result

def compute_hash(file_path):
    """Compute SHA-256 hash of a file"""
    with open(file_path, 'rb') as f:
        return hashlib.sha256(f.read()).hexdigest()

def copy_with_info(src_path, dst_path, implementation):
    """Copy file and print info"""
    if src_path.exists():
        shutil.copy2(src_path, dst_path)
        file_size = dst_path.stat().st_size
        hash_val = compute_hash(dst_path)
        print(f"✅ {implementation:>3} | {dst_path.name:<25} | {file_size:>8} bytes | {hash_val[:12]}...")
        return True
    else:
        print(f"❌ {implementation:>3} | {src_path.name:<25} | NOT FOUND")
        return False

def main():
    script_root = Path(__file__).parent
    project_root = script_root.parent
    c_dir = project_root / "src/c"
    
    # Create output directories
    output_dir = project_root / "output"
    c_dir_out = output_dir / "c"
    asm_dir_out = output_dir / "asm"
    
    for dir_path in [output_dir, c_dir_out, asm_dir_out]:
        dir_path.mkdir(exist_ok=True)
    
    print("🎵 NotDeafbeef Comprehensive WAV Test Generator")
    print("=" * 70)
    print(f"Building all audio targets in both C and ASM implementations")
    print(f"Output directory: {output_dir}")
    print()
    
    # Track results
    c_results = {}
    asm_results = {}
    
    for target, wav_files in AUDIO_TARGETS:
        print(f"🔨 Building target: {target}")
        
        # Ensure wav_files is always a list
        if isinstance(wav_files, str):
            wav_files = [wav_files]
        
        # Build C implementation
        print(f"   Building C implementation...")
        c_result = run_make_command(c_dir, target, use_asm=False)
        
        if c_result.returncode == 0:
            # Copy C WAV files
            c_success = []
            for wav_file in wav_files:
                src_path = c_dir / wav_file
                dst_path = c_dir_out / f"c_{wav_file}"
                success = copy_with_info(src_path, dst_path, "C")
                c_success.append(success)
            c_results[target] = all(c_success)
        else:
            print(f"❌ C build failed for {target}")
            if c_result.stderr:
                print(f"   Error: {c_result.stderr[:200]}...")
            c_results[target] = False
        
        # Build ASM implementation  
        print(f"   Building ASM implementation...")
        asm_result = run_make_command(c_dir, target, use_asm=True)
        
        if asm_result.returncode == 0:
            # Copy ASM WAV files
            asm_success = []
            for wav_file in wav_files:
                src_path = c_dir / wav_file
                dst_path = asm_dir_out / f"asm_{wav_file}"
                success = copy_with_info(src_path, dst_path, "ASM")
                asm_success.append(success)
            asm_results[target] = all(asm_success)
        else:
            print(f"❌ ASM build failed for {target}")
            if asm_result.stderr:
                print(f"   Error: {asm_result.stderr[:200]}...")
            asm_results[target] = False
        
        print()
    
    # Summary
    print("=" * 70)
    print("📊 SUMMARY")
    print("=" * 70)
    
    c_success_count = sum(1 for success in c_results.values() if success)
    asm_success_count = sum(1 for success in asm_results.values() if success)
    total_targets = len(AUDIO_TARGETS)
    
    print(f"C Implementation:   {c_success_count:>2}/{total_targets} targets successful")
    print(f"ASM Implementation: {asm_success_count:>2}/{total_targets} targets successful")
    print()
    
    # Detailed results
    print("Detailed Results:")
    print("Target               | C   | ASM | Notes")
    print("-" * 50)
    
    for target, _ in AUDIO_TARGETS:
        c_status = "✅" if c_results.get(target, False) else "❌"
        asm_status = "✅" if asm_results.get(target, False) else "❌"
        
        notes = ""
        if c_results.get(target, False) and asm_results.get(target, False):
            notes = "Both working"
        elif c_results.get(target, False):
            notes = "C only"
        elif asm_results.get(target, False):
            notes = "ASM only"
        else:
            notes = "Both failed"
            
        print(f"{target:<20} | {c_status}  | {asm_status}  | {notes}")
    
    print()
    print(f"🎧 WAV files saved to:")
    print(f"   C implementation:   {c_dir_out}")
    print(f"   ASM implementation: {asm_dir_out}")
    print()
    print("🔍 Next steps:")
    print("   1. Audition the WAV files to verify they sound correct")
    print("   2. Compare C vs ASM versions for each sound")
    print("   3. Investigate any failed builds")

if __name__ == "__main__":
    main() 