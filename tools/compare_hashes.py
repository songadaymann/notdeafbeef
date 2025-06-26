#!/usr/bin/env python3
"""
Compare current WAV file hashes with baseline hashes to show differences.
"""

from pathlib import Path
from tests.hash_wav import hash_wav

# Mapping of audit WAV files to their baseline hash files
COMPARISONS = [
    ("current_bass_only.wav", "bass_hash.txt", "Bass"),
    ("current_bass_quantum.wav", "bass_quantum_hash.txt", "Bass Quantum"), 
    ("current_bass_plucky.wav", "bass_plucky_hash.txt", "Bass Plucky"),
    ("current_fm.wav", "fm_hash.txt", "FM"),
    ("current_bells-c.wav", "bells_c_hash.txt", "Bells"),
    ("current_hat.wav", "hat_hash.txt", "Hat"),
    ("current_snare.wav", "snare_hash.txt", "Snare"),
    ("current_kick.wav", "kick_hash.txt", "Kick"),
    ("current_melody.wav", "melody_hash.txt", "Melody"),
    ("current_sine.wav", "sine_hash.txt", "Sine"),
    ("current_saw.wav", "saw_hash.txt", "Saw"),
    ("current_square.wav", "square_hash.txt", "Square"),
    ("current_triangle.wav", "triangle_hash.txt", "Triangle"),
    ("current_calm-c.wav", "calm_c_hash.txt", "Calm"),
    ("current_quantum-c.wav", "quantum_c_hash.txt", "Quantum"),
    ("current_pluck-c.wav", "pluck_c_hash.txt", "Pluck"),
]

def main():
    root = Path(__file__).parent
    audit_dir = root / "audit_wavs"
    baseline_dir = root / "tests" / "baseline"
    
    print("🔍 Comparing current assembly output with baseline hashes...\n")
    
    matches = 0
    total = 0
    
    for wav_file, baseline_file, name in COMPARISONS:
        wav_path = audit_dir / wav_file
        baseline_path = baseline_dir / baseline_file
        
        if wav_path.exists() and baseline_path.exists():
            # Calculate current hash
            current_hash = hash_wav(wav_path, mode="coarse")
            baseline_hash = baseline_path.read_text().strip()
            
            # Compare
            if current_hash == baseline_hash:
                print(f"✅ {name:<15} MATCH")
                matches += 1
            else:
                print(f"❌ {name:<15} DIFFERENT")
                print(f"   Current:  {current_hash[:32]}...")
                print(f"   Baseline: {baseline_hash[:32]}...")
            
            total += 1
            
        elif not wav_path.exists():
            print(f"⚠️  {name:<15} WAV file missing")
        elif not baseline_path.exists():
            print(f"⚠️  {name:<15} Baseline missing")
    
    print(f"\n📊 Summary: {matches}/{total} matches")
    
    if matches < total:
        print(f"\n🎧 Listen to the WAV files in audit_wavs/ directory")
        print(f"If they sound good, run: python update_baselines.py")

if __name__ == "__main__":
    main() 