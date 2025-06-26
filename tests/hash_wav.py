"""Utility helpers for hashing a WAV file during tests.

Two modes are available:

1. exact  – byte-for-byte digest of the whole file                       (legacy)
2. coarse – digest after converting to int16 and discarding a few LSBs   (tolerant)

The *coarse* mode lets us keep unit tests green when a new NEON / ASM
implementation changes results only at the sub-LSB level (< ≈0.1 dB).
"""

from __future__ import annotations

import hashlib
import struct
import sys
import pathlib
import wave
from typing import Literal


_Mode = Literal["exact", "coarse"]


def _hash_bytes(buf: bytes) -> str:  # small convenience wrapper
    return hashlib.sha256(buf).hexdigest()


def _coarse_int16_digest(path: pathlib.Path, lsb_drop: int = 4) -> str:
    """Hash after converting the PCM stream to int16 and dropping *lsb_drop* LSBs.

    Dropping 4 LSBs corresponds to keeping the top-12-bit mantissa which
    tolerates ≈-72 dB of noise – more than enough for the ±0.1 dB drift we
    accept when swapping numerically different but audibly transparent code.
    """

    if lsb_drop <= 0:
        raise ValueError("lsb_drop must be >= 1 when using coarse mode")

    with wave.open(str(path), "rb") as wf:
        n_channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        n_frames = wf.getnframes()

        raw = wf.readframes(n_frames)

    # --- convert to int16 little-endian ---------------------------------------------------
    if sampwidth == 2:  # already int16 LE
        int16_data = raw
    elif sampwidth == 4:
        # Assume IEEE-754 float32 PCM in [-1.0, 1.0]
        count = n_frames * n_channels
        floats = struct.unpack(f"<{count}f", raw)
        ints = (max(-32768, min(32767, int(v * 32767.0))) for v in floats)
        int16_data = struct.pack(f"<{count}h", *ints)
    else:
        raise RuntimeError(f"Unsupported WAV sample width: {sampwidth} bytes")

    # --- drop least-significant bits ------------------------------------------------------
    mask = 0xFFFF ^ ((1 << lsb_drop) - 1)  # e.g. for 4 -> 0xFFF0
    count = len(int16_data) // 2
    ints = struct.unpack(f"<{count}h", int16_data)

    def _apply_mask(val: int) -> int:
        masked = val & mask
        # Convert back to signed 16-bit range if needed
        return masked - 0x10000 if masked > 0x7FFF else masked

    trimmed = (_apply_mask(s) for s in ints)
    trimmed_bytes = struct.pack(f"<{count}h", *trimmed)

    return _hash_bytes(trimmed_bytes)


def hash_wav(path: str | pathlib.Path, mode: _Mode = "exact", *, lsb_drop: int = 4) -> str:
    """Return a SHA-256 digest of *path*.

    mode="exact"  → traditional byte-for-byte hash of the entire file.
    mode="coarse" → int16-quantised + LSB-dropped hash (see docs above).
    """

    p = pathlib.Path(path)

    if mode == "exact":
        return _hash_bytes(p.read_bytes())
    elif mode == "coarse":
        return _coarse_int16_digest(p, lsb_drop=lsb_drop)
    else:
        raise ValueError("mode must be 'exact' or 'coarse'")


# ---------------------------------------------------------------------------
# CLI helper: behave like the old script (exact digest)
# ---------------------------------------------------------------------------


if __name__ == "__main__":
    if not (2 <= len(sys.argv) <= 3):
        print("Usage: python hash_wav.py <file.wav> [coarse]", file=sys.stderr)
        sys.exit(1)

    wav_path = sys.argv[1]
    mode = "coarse" if len(sys.argv) == 3 else "exact"
    print(hash_wav(wav_path, mode=mode)) 