import subprocess
from pathlib import Path
import pytest

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "bass_only.wav"
BASE = ROOT / "tests" / "baseline" / "bass_hash.txt"

PRESETS = [
    ("bass", "bass_only.wav", "bass_hash.txt"),
    ("bass_quantum", "bass_quantum.wav", "bass_quantum_hash.txt"),
    ("bass_plucky", "bass_plucky.wav", "bass_plucky_hash.txt"),
]

def test_bass_hash():
    # Build and render the bass-only WAV
    subprocess.run(["make", "-C", str(CVER), "clean", "bass"], check=True)
    assert WAV.exists(), "bass_only.wav missing after build"
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f"bass_only.wav hash mismatch: got {h}, expected {expected}"

@pytest.mark.parametrize("make_target, wav_name, baseline_file", PRESETS)
def test_bass_hash_parametrized(make_target, wav_name, baseline_file):
    # Build and render the bass-only WAV
    subprocess.run(["make", "-C", str(CVER), "clean", make_target], check=True)
    wav_path = CVER / wav_name
    assert wav_path.exists(), f"{wav_name} missing after build"
    h = hash_wav(wav_path, mode="coarse")
    baseline_path = ROOT / "tests" / "baseline" / f"{baseline_file}"
    expected = baseline_path.read_text().strip()
    assert h == expected, f"{wav_name} hash mismatch: got {h}, expected {expected}" 