import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "kick.wav"
BASE = ROOT / "tests" / "baseline" / "kick_hash.txt"

def test_kick_hash():
    subprocess.run(["make", "-C", str(CVER), "clean", "kick"], check=True)
    assert WAV.exists()
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected 