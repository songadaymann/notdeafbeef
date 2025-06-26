import subprocess, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"

# Map: make target -> wav path -> baseline file
WAV_TARGETS = [
    ("sine", CVER / "sine.wav", ROOT / "tests" / "baseline" / "sine_hash.txt"),
    ("tones", CVER / "saw.wav", ROOT / "tests" / "baseline" / "saw_hash.txt"),
    ("tones", CVER / "square.wav", ROOT / "tests" / "baseline" / "square_hash.txt"),
    ("tones", CVER / "triangle.wav", ROOT / "tests" / "baseline" / "triangle_hash.txt"),
]


def wav_sha256(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


import pytest

@pytest.mark.parametrize("make_target, wav_path, baseline", WAV_TARGETS)
def test_hashes(make_target: str, wav_path: Path, baseline: Path):
    subprocess.run(["make", "-C", str(CVER), make_target, "-j4"], check=True)
    assert wav_path.exists(), f"{wav_path.name} missing after build"
    h = wav_sha256(wav_path)
    expected = baseline.read_text().strip()
    assert h == expected, f"{wav_path.name} hash mismatch: got {h}, expected {expected}" 