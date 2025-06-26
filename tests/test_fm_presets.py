import subprocess
from pathlib import Path
import pytest

from tests.hash_wav import hash_wav

ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / "C-version"

PRESETS = [
    ("bells", "bells-c.wav", "bells_c_hash.txt"),
    ("calm", "calm-c.wav", "calm_c_hash.txt"),
    ("quantum", "quantum-c.wav", "quantum_c_hash.txt"),
    ("pluck", "pluck-c.wav", "pluck_c_hash.txt"),
]


@pytest.mark.parametrize("make_target, wav_name, baseline_file", PRESETS)
def test_fm_preset_hash(make_target: str, wav_name: str, baseline_file: str):
    subprocess.run(["make", "-C", str(CVER), "clean", make_target], check=True)
    wav_path = CVER / wav_name
    assert wav_path.exists(), f"{wav_name} missing after build"
    h = hash_wav(wav_path, mode="coarse")
    expected = (ROOT / "tests" / "baseline" / baseline_file).read_text().strip()
    assert h == expected, f"{wav_name} hash mismatch: got {h}, expected {expected}" 