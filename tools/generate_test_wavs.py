#!/usr/bin/env python3
"""
Generate WAV files for all the failing hash tests so they can be auditioned.
This script builds each target and copies the resulting WAV to an 'audit' directory.
"""

import subprocess
import shutil
import pathlib
from pathlib import Path

# Test targets that were failing hash tests
TARGETS = [
    ("bass", "bass_only.wav"),
    ("bass_quantum", "bass_quantum.wav"), 
    ("bass_plucky", "bass_plucky.wav"),
    ("fm", "fm.wav"),
    ("bells", "bells-c.wav"),
    ("hat", "hat.wav"),
    ("snare", "snare.wav"),
    ("kick", "kick.wav"),
    ("melody", "melody.wav"),
]

def main():
    root = Path(__file__).parent
    c_version = root / "src/c"
    audit_dir = root / "output/audit"
    
    # Create audit directory
    audit_dir.mkdir(exist_ok=True)
    
    print(f"Generating WAV files for audition in {audit_dir}")
    
    for make_target, wav_name in TARGETS:
        print(f"\n🎵 Building {make_target} -> {wav_name}")
        
        try:
            # Build the target (already includes clean)
            subprocess.run(["make", "-C", str(c_version), "clean", make_target], 
                          check=True, capture_output=True)
            
            # Check if WAV was created
            wav_path = c_version / wav_name
            if wav_path.exists():
                # Copy to audit directory with descriptive name
                audit_path = audit_dir / f"current_{wav_name}"
                shutil.copy2(wav_path, audit_path)
                print(f"✅ Generated: {audit_path}")
                
                # Also show the hash for reference
                import hashlib
                with open(audit_path, 'rb') as f:
                    hash_val = hashlib.sha256(f.read()).hexdigest()
                print(f"   Hash: {hash_val[:16]}...")
            else:
                print(f"❌ WAV file not found: {wav_path}")
                
        except subprocess.CalledProcessError as e:
            print(f"❌ Build failed for {make_target}")
            if e.stderr:
                print(f"   Error: {e.stderr.decode()[:200]}...")
    
    print(f"\n🎧 All WAV files saved to: {audit_dir}")
    print("You can now audition these files to check if they sound correct!")

if __name__ == "__main__":
    main() 