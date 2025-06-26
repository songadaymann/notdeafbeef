import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "fm-c.wav"
BASE = ROOT / "tests" / "baseline" / "fm_c_hash.txt"

def test_fm_hash():
    subprocess.run(["make", "-C", str(CVER), "clean", "fm"], check=True)
    assert WAV.exists(), "fm.wav missing"
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f"fm.wav hash mismatch: got {h}, expected {expected}" 