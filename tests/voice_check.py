#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Measure every voice on its own (tests/voice_probe.c): its level, and for the sampled voices its tuning.
  python3 tests/voice_check.py        (after building build/host/voice_probe; exits 1 on a silent or mistuned voice)"""
import os, subprocess, sys, tempfile
import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
PROBE = os.path.join(HERE, "..", "build", "host", "voice_probe")
N = {"bass": 16, "keys": 25, "lead": 28, "drum": 13}
REAL = {"bass": [14, 15], "keys": [18, 19, 20, 21], "lead": [25, 26, 27]}   # (the tonewheels are exact sines)


def run(kind, inst, note, secs=1.2, *extra):
    with tempfile.NamedTemporaryFile(suffix=".s16") as t:
        subprocess.run([PROBE, kind, str(inst), str(note), str(secs), t.name, *map(str, extra)], check=True)
        return np.fromfile(t.name, "<i2").astype(float)


def f0(x, fs=44100, lo=30, hi=1500):
    seg = x[int(0.25 * fs):int(0.75 * fs)]
    seg = seg - seg.mean()
    n = len(seg)
    ac = np.fft.irfft(np.abs(np.fft.rfft(seg, 2 * n)) ** 2)[:n] / np.arange(n, 0, -1)
    l0, l1 = int(fs / hi), int(fs / lo)
    k = l0 + int(np.argmax(ac[l0:l1]))
    thr = 0.9 * ac[k]
    for j in range(l0 + 1, k):
        if ac[j] > thr and ac[j] >= ac[j - 1] and ac[j] >= ac[j + 1]:
            k = j
            break
    d = ac[k - 1] - 2 * ac[k] + ac[k + 1]
    return fs / (k + (0.5 * (ac[k - 1] - ac[k + 1]) / d if d else 0))


def main():
    bad = 0
    for kind, note in (("bass", 45), ("keys", 69), ("lead", 69)):
        for i in range(N[kind]):
            x = run(kind, i, note)
            rms, peak = np.sqrt((x ** 2).mean()), np.abs(x).max()
            line = f"{kind} {i:2d}  rms {rms:6.0f}  peak {peak:6.0f}"
            if i in REAL[kind]:
                # every zone of a sampled voice: in tune to 15 cents (the real basses sound an octave down)
                worst = 0.0
                notes = range(40, 65, 3) if kind == "bass" else range(45, 85, 4) if kind == "keys" else range(48, 84, 3)
                for nt in notes:
                    y = run(kind, i, nt)
                    want = 440.0 * 2 ** (((nt - 12 if kind == "bass" else nt) - 69) / 12)
                    got = f0(y, lo=want / 1.4, hi=want * 1.4)
                    cents = 1200 * np.log2(got / want)
                    worst = max(worst, abs(cents))
                line += f"  tuning within {worst:4.1f} cents"
                if worst > 15:
                    line += "  <-- MISTUNED"; bad += 1
            if rms < 300:
                line += "  <-- SILENT"; bad += 1
            print(line)
    for kit in (0, 12):
        for v in range(9):
            x = run("drum", kit, v, 1.0)
            print(f"drum kit {kit:2d} voice {v}  rms {np.sqrt((x ** 2).mean()):6.0f}  peak {np.abs(x).max():6.0f}"
                  f"  length {np.nonzero(np.abs(x) > 60)[0].max() / 44100 if np.abs(x).max() > 60 else 0:.2f} s")
            if np.abs(x).max() < 500:
                bad += 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
