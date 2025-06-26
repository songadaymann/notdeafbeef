#!/usr/bin/env python3
"""Benchmark the FM generator executable.

Usage:
    python bench_gen_fm.py [iterations]

The script will:
  1. Ensure the `gen_fm` target is built (in release settings) by
     invoking `make -C C-version fm -jN`.
  2. Run the resulting binary N times (default 10) while measuring
     wall-clock time using `time.perf_counter()`.
  3. Report the per-run timings and the median, so we can compare C vs
     assembly implementations going forward.

The benchmark performs no I/O beyond what `gen_fm` already does.  Any
generated .wav files are left in place for potential inspection.
"""
from __future__ import annotations

import subprocess
import sys
import time
from pathlib import Path
from statistics import median
import os

ROOT = Path(__file__).resolve().parent
CVER = ROOT / "C-version"
BIN = CVER / "bin" / "gen_fm"


def build() -> None:
    """Build the `gen_fm` executable using Make."""
    print("[bench] building gen_fm …", flush=True)
    cpu_count = str(max(1, (os.cpu_count() or 1)))
    subprocess.run(["make", "-C", str(CVER), "fm", f"-j{cpu_count}"], check=True)


def time_once() -> float:
    """Run the binary once, return elapsed wall-clock time in seconds."""
    start = time.perf_counter()
    subprocess.run([str(BIN)], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    return time.perf_counter() - start


def main(argv: list[str]) -> None:
    iterations = int(argv[1]) if len(argv) > 1 else 10
    build()

    print(f"[bench] running {iterations} iteration(s)…")
    times: list[float] = []
    for i in range(iterations):
        t = time_once()
        times.append(t)
        print(f"  run {i+1:02}/{iterations}: {t:.3f} s", flush=True)

    print("[bench] median runtime: {:.3f} s".format(median(times)))


if __name__ == "__main__":
    main(sys.argv) 