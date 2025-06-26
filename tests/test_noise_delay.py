import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "delay.wav"
BASE = ROOT / "tests" / "baseline" / "delay_hash.txt"


def test_delay_hash():
    # Build delay.wav via make target (includes asm delay + noise)
    subprocess.run(["make", "-C", str(CVER), "clean", "delay"], check=True)
    assert WAV.exists(), "delay build did not create wav"
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f"WAV hash mismatch: got {h}, expected {expected}" 