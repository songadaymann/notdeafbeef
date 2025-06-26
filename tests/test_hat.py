import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "hat.wav"
BASE = ROOT / "tests" / "baseline" / "hat_hash.txt"

def test_hat_hash():
    subprocess.run(["make", "-C", str(CVER), "clean", "hat"], check=True)
    assert WAV.exists(), "hat.wav missing after build"
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f"hat.wav hash mismatch: got {h}, expected {expected}" 