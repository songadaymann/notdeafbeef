import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "snare.wav"
BASE = ROOT / "tests" / "baseline" / "snare_hash.txt"

def test_snare_hash():
    subprocess.run(["make", "-C", str(CVER), "clean", "snare"], check=True)
    assert WAV.exists(), "snare.wav missing after build"
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f"snare.wav hash mismatch: got {h}, expected {expected}" 