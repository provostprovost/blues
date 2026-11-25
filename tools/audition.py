#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Render the factory presets on this computer, to listen to (no FM-1 needed).

  python3 tools/audition.py [PRESET ...] [--bars N] [--no-lead]

For each preset (1..16, default all): the host build of the firmware (tests/host.c) loads the preset, presses PLAY,
plays a chorus with a short lead phrase on the keys, and stops. Out comes build/audition/preset-NN.wav.
"""
import argparse
import os
import subprocess
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "build", "audition")
HOST = os.path.join(ROOT, "build", "host", "dinghy")
WHITE = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26]
BPM = [118, 62, 126, 138, 96, 76, 104, 118, 156, 100, 94, 98, 78, 128, 96, 122]
BTN_GLO, BTN_PLAY = 7, 12
# a lead phrase as (beat, length in beats, key index 0..26 = F3..G5); Easy Lead or not, it stays close to the key
PHRASE = [(0.0, 1.5, 11), (1.5, 0.5, 14), (2.0, 1.0, 16), (3.0, 0.5, 14), (3.5, 2.5, 11),
          (8.0, 0.5, 16), (8.5, 0.5, 18), (9.0, 2.0, 21), (11.0, 0.5, 18), (11.5, 0.5, 16), (12.0, 3.0, 14)]


def script(preset, bars, lead):
    bpm = BPM[preset - 1]
    beat = 60000.0 / bpm
    ev = [(100, f"btndown {BTN_GLO}"), (200, f"keys {1 << WHITE[preset - 1]}"), (300, "keys 0"),
          (400, f"btnup {BTN_GLO}"), (700, f"btn {BTN_PLAY}")]
    t0 = 700 + 4 * beat * 2          # (past the lead-in)
    if lead:
        for rep in range(max(1, bars // 8)):
            for b, n, k in PHRASE:
                on = t0 + (rep * 32 + 16 + b) * beat
                if on + n * beat > 700 + (bars + 1) * 4 * beat:
                    continue
                ev.append((int(on), f"keys {1 << k}"))
                ev.append((int(on + n * beat * 0.92), "keys 0"))
    end = int(700 + (bars + 2) * 4 * beat)
    ev.append((end, f"btn {BTN_PLAY}"))
    ev.append((end + 1500, "end"))
    return "".join(f"{t} {what}\n" for t, what in sorted(ev))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("presets", nargs="*", type=int)
    ap.add_argument("--bars", type=int, default=12)
    ap.add_argument("--no-lead", action="store_true")
    a = ap.parse_args()
    os.makedirs(OUT, exist_ok=True)
    os.makedirs(os.path.dirname(HOST), exist_ok=True)
    subprocess.run(["cc", "-O2", "-Wall", "-Wno-unused-function", "-DDINGHY_HOST", '-DAPP_VER="dev"',
                    "-I" + os.path.join(ROOT, "firmware", "hal"), "-I" + os.path.join(ROOT, "firmware", "src"),
                    "-I" + os.path.join(ROOT, "firmware", "gen"), "-o", HOST,
                    os.path.join(ROOT, "firmware", "src", "dinghy.c")], check=True)
    for p in a.presets or range(1, 17):
        base = os.path.join(OUT, f"preset-{p:02d}")
        with open(base + ".txt", "w") as f:
            f.write(script(p, a.bars, not a.no_lead))
        subprocess.run([HOST, base, base + ".txt"], check=True, capture_output=True)
        print(base + ".wav")


if __name__ == "__main__":
    sys.exit(main())
