#!/usr/bin/env python3
"""
Update baseline hashes after auditioning WAV files.
Run this after you've listened to the WAV files in audit_wavs/ and confirmed they sound good.
"""

import subprocess
from pathlib import Path
from tests.hash_wav import hash_wav

# Mapping of audit WAV files to their baseline hash files
BASELINE_MAPPINGS = [
    ("current_bass_only.wav", "bass_hash.txt"),
    ("current_bass_quantum.wav", "bass_quantum_hash.txt"), 
    ("current_bass_plucky.wav", "bass_plucky_hash.txt"),
    ("current_fm.wav", "fm_hash.txt"),
    ("current_bells-c.wav", "bells_c_hash.txt"),
    ("current_hat.wav", "hat_hash.txt"),
    ("current_snare.wav", "snare_hash.txt"),
    ("current_kick.wav", "kick_hash.txt"),
    ("current_melody.wav", "melody_hash.txt"),
    ("current_sine.wav", "sine_hash.txt"),
    ("current_saw.wav", "saw_hash.txt"),
    ("current_square.wav", "square_hash.txt"),
    ("current_triangle.wav", "triangle_hash.txt"),
    ("current_calm-c.wav", "calm_c_hash.txt"),
    ("current_quantum-c.wav", "quantum_c_hash.txt"),
    ("current_pluck-c.wav", "pluck_c_hash.txt"),
]

def main():
    root = Path(__file__).parent
    audit_dir = root / "audit_wavs"
    baseline_dir = root / "tests" / "baseline"
    
    print("🎧 Updating baseline hashes with current assembly output...")
    print("Make sure you've auditioned the files and they sound good!\n")
    
    for wav_file, baseline_file in BASELINE_MAPPINGS:
        wav_path = audit_dir / wav_file
        baseline_path = baseline_dir / baseline_file
        
        if wav_path.exists():
            # Calculate new hash using the same "coarse" mode as tests
            new_hash = hash_wav(wav_path, mode="coarse")
            
            # Read old hash if it exists
            old_hash = "none"
            if baseline_path.exists():
                old_hash = baseline_path.read_text().strip()
            
            # Update the baseline
            baseline_path.write_text(new_hash + "\n")
            
            print(f"✅ {baseline_file}")
            print(f"   Old: {old_hash[:16]}...")
            print(f"   New: {new_hash[:16]}...")
            
            # Also copy the WAV file to baseline for auditioning [per user memory]
            baseline_wav = baseline_dir / wav_file.replace("current_", "baseline_")
            if not baseline_wav.exists():
                import shutil
                shutil.copy2(wav_path, baseline_wav)
                print(f"   📁 Saved baseline WAV: {baseline_wav.name}")
            
        else:
            print(f"❌ Missing: {wav_path}")
    
    print(f"\n🎯 Updated {len(BASELINE_MAPPINGS)} baseline hashes!")
    print("You can now run 'make test' to see if the tests pass.")

if __name__ == "__main__":
    main() 