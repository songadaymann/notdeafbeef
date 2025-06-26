import subprocess
from pathlib import Path

from tests.hash_wav import hash_wav  # tolerant / exact digest helper


ROOT = Path(__file__).resolve().parent.parent
CVER = ROOT / 'C-version'
WAV = CVER / 'seed_0xcafebabe.wav'
BASE = ROOT / 'tests' / 'baseline' / 'segment_hash.txt'


def test_segment_hash():
    # Build segment wav
    subprocess.run(['make', '-C', str(CVER), 'clean', 'segment'], check=True)
    assert WAV.exists(), 'segment build did not create wav'
    # Use coarse mode to tolerate sub-LSB drift in large mixes
    h = hash_wav(WAV, mode="coarse")
    expected = BASE.read_text().strip()
    assert h == expected, f'WAV hash mismatch: got {h}, expected {expected}' 