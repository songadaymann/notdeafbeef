import subprocess, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"
WAV = CVER / "melody.wav"
BASE = ROOT / "tests" / "baseline" / "melody_hash.txt"

def wav_sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()

def test_melody_hash():
    subprocess.run(["make", "-C", str(CVER), "melody", "-j4"], check=True)
    assert WAV.exists(), "melody.wav missing"
    h = wav_sha256(WAV)
    expected = BASE.read_text().strip()
    assert h == expected, f"melody.wav hash mismatch: got {h}, expected {expected}" 