#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build Blues Box's sample bank (firmware/gen/sample_bank.h) from the free sample libraries in samples-src/.

  python3 -I tools/build_samples.py [--scale 1.0] [--wav DIR]

The sources (all CC0, see LICENSING.md; download them into samples-src/, which is never committed):
  samples-src/growlybass   Karoryfer Growlybass (a Squier Jazz bass)
  samples-src/meatbass     Karoryfer Meatbass (a 1958 Otto Rubner double bass)
  samples-src/guitars      Karoryfer Black And Green Guitars (two hollowbody electrics)
  samples-src/virtuosity   Versilian / Karoryfer Virtuosity Drums (a jazz kit)
  samples-src/vcsl/repo    Versilian Community Sample Library (the tenor saxophone)

A note of a pitched instrument: mono, trimmed at its onset, resampled to the zone's rate, its decay taken out after
the attack (the firmware puts a decay of its own back), a loop of a few periods crossfaded into itself, then block
ADPCM (tools/erkcodec.c: Erik37's codec, 4.25 bits a sample). Drums are one-shots with a fade at the end.
--scale stretches or shrinks every duration (the bank's size); --wav writes each zone as decoded, to listen to."""
import argparse
import os
import re
import struct
import subprocess
import sys
import tempfile
import wave
from fractions import Fraction

import numpy as np
from scipy import signal

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "samples-src")
OUT = os.path.join(HERE, "..", "firmware", "gen", "sample_bank.h")
BUILD = os.path.join(HERE, "..", "build", "samples")
CODEC = os.path.join(BUILD, "erkcodec")
FS = 44100
NOTE = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}


def midi_hz(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def read_audio(path):
    """-> (mono float64 in -1..1, rate); FLAC through macOS afconvert / flac / ffmpeg, whichever is there"""
    tmp = None
    if not path.lower().endswith(".wav"):
        tmp = tempfile.NamedTemporaryFile(suffix=".wav", delete=False).name
        for cmd in (["afconvert", "-f", "WAVE", "-d", "LEI16", path, tmp], ["flac", "-d", "-f", "-o", tmp, path],
                    ["ffmpeg", "-y", "-i", path, tmp]):
            try:
                if subprocess.run(cmd, capture_output=True).returncode == 0:
                    break
            except FileNotFoundError:
                continue
        else:
            raise SystemExit(f"cannot decode {path}: need afconvert, flac or ffmpeg")
        path = tmp
    d = open(path, "rb").read()
    if tmp:
        os.unlink(tmp)
    if d[:4] != b"RIFF" or d[8:12] != b"WAVE":
        raise SystemExit(f"{path}: not a WAV file")
    ch = w = fs = 0
    raw = b""
    i = 12
    while i + 8 <= len(d):                              # (the RIFF chunks: PCM, also as WAVE_FORMAT_EXTENSIBLE)
        tag, n = d[i:i + 4], struct.unpack("<I", d[i + 4:i + 8])[0]
        if tag == b"fmt ":
            _, ch, fs, _, _, bits = struct.unpack("<HHIIHH", d[i + 8:i + 24])
            w = bits // 8
        elif tag == b"data":
            raw = d[i + 8:i + 8 + n]
        i += 8 + n + (n & 1)
    raw = raw[:len(raw) - len(raw) % (ch * w)]
    b = np.frombuffer(raw, np.uint8)
    if w == 2:
        x = b.view("<i2").astype(np.float64) / 32768
    elif w == 3:
        t = b.reshape(-1, 3).astype(np.int32)
        v = t[:, 2] << 16 | t[:, 1] << 8 | t[:, 0]
        x = np.where(v >= 1 << 23, v - (1 << 24), v).astype(np.float64) / (1 << 23)
    elif w == 4:
        x = b.view("<i4").astype(np.float64) / 2 ** 31
    else:
        raise SystemExit(f"{path}: {w}-byte samples")
    return x.reshape(-1, ch).mean(axis=1), fs


def trim_onset(x, fs, rel=0.03, pre_ms=1.5):
    a = np.abs(x)
    i = int(np.argmax(a > rel * a.max()))
    i = max(0, i - int(pre_ms * fs / 1000))
    y = x[i:].copy()
    n = min(len(y), int(0.001 * fs))
    y[:n] *= np.linspace(0, 1, n)
    return y


def resample(x, fs, rate):
    if fs == rate:
        return x
    fr = Fraction(rate, fs).limit_denominator(1000)
    return signal.resample_poly(x, fr.numerator, fr.denominator)


def name_midi(path):
    """the note in a file name ('eb3', 'A#1'), as written: the library's octave offset is found by the caller"""
    m = re.search(r"(?:^|_)([a-gA-G])(#|b)?(-?\d)(?:_|\.)", os.path.basename(path))
    if not m:
        return None
    return 12 * (int(m.group(3)) + 1) + NOTE[m.group(1).lower()] + {"#": 1, "b": -1, None: 0}[m.group(2)]


def detect_f0(x, fs, lo=30.0, hi=1200.0, near=None):
    """the pitch of a note's steady part, by autocorrelation; near (Hz): only within 1.5 semitones of it"""
    if near:
        lo, hi = near * 2 ** (-1.5 / 12), near * 2 ** (1.5 / 12)
    a, b = int(0.12 * fs), int(0.45 * fs)
    seg = x[a:b] if len(x) > b else x[len(x) // 4:]
    seg = seg - seg.mean()
    n = len(seg)
    ac = signal.fftconvolve(seg, seg[::-1])[n - 1:]
    l0, l1 = int(fs / hi), min(int(fs / lo), n - 1)
    ac = ac / (np.arange(n, 0, -1))                   # (unbiased)
    k = l0 + int(np.argmax(ac[l0:l1]))
    # the first strong peak: a peak at a multiple of the period is as high as the period's own
    thr = 0.85 * ac[k]
    for j in range(l0 + 1, k if not near else l0):
        if ac[j] > thr and ac[j] >= ac[j - 1] and ac[j] >= ac[j + 1]:
            k = j
            break
    if 0 < k < n - 1:                                  # parabolic refinement
        d = ac[k - 1] - 2 * ac[k] + ac[k + 1]
        if d != 0:
            return fs / (k + 0.5 * (ac[k - 1] - ac[k + 1]) / d)
    return fs / k


def envelope(x, fs, ms=25.0):
    n = max(1, int(ms * fs / 1000))
    return np.sqrt(signal.fftconvolve(x * x, np.ones(n) / n, mode="same") + 1e-12)


def encode(y16):
    """block ADPCM -> (bytes, decoded int16)"""
    with tempfile.TemporaryDirectory() as d:
        a, b, c = (os.path.join(d, n) for n in ("in.s16", "out.bin", "dec.s16"))
        y16.astype("<i2").tofile(a)
        subprocess.run([CODEC, "enc", a, b, c, "16"], check=True, capture_output=True)
        return open(b, "rb").read(), np.fromfile(c, "<i2")


class Bank:
    def __init__(self, wav_dir):
        self.data = bytearray()
        self.zones = []          # dicts
        self.wav_dir = wav_dir

    def add(self, name, y, rate, loop=None, f0=None, root=0, hi=127, gain=1.0):
        """y: float mono at rate, already cut; loop: (start, end) in samples or None"""
        y = y / (np.abs(y).max() + 1e-12) * 0.92
        if loop:
            ls, le = loop
            pad = (-le) % 32
            y = np.concatenate([y[:le], y[ls:ls + pad]])
        else:
            pad = (-len(y)) % 32
            y = np.concatenate([y, np.zeros(pad)])
        y16 = np.clip(np.round(y * 32767), -32768, 32767).astype(np.int16)
        blob, dec = encode(y16)
        z = {"name": name, "off": len(self.data), "len": (loop[1] if loop else len(y16) - pad), "loop": 0,
             "loop_len": 0, "h1": 0, "h2": 0, "root": root, "hi": hi, "gain": int(round(gain * 4096)),
             "bytes": len(blob), "rate": rate}
        if loop:
            ls, le = loop
            z.update(loop=ls, loop_len=le - ls, h1=int(dec[ls - 1]), h2=int(dec[ls - 2]))
            z["k"] = int(round(256.0 * rate / f0))
        else:
            z["k"] = int(round(65536.0 * rate / FS))
        err = y16.astype(np.float64) - dec[:len(y16)]
        z["snr"] = 10 * np.log10((y16.astype(np.float64) ** 2).sum() / ((err ** 2).sum() + 1e-9))
        self.data += blob
        self.zones.append(z)
        if self.wav_dir:
            out = dec[:len(y16)]
            if loop:
                out = np.concatenate([dec[:le]] + [dec[ls:le]] * 6)
            w = wave.open(os.path.join(self.wav_dir, re.sub(r"[^\w]+", "_", name) + ".wav"), "wb")
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
            w.writeframes(out.astype("<i2").tobytes()); w.close()
        return len(self.zones) - 1


def make_loop(y, rate, f0, t_att, t_loop, flatten=True):
    """-> (y cut at the loop's end, loop start, loop end). The loop starts on a block (32 samples), is a whole
    number of periods long, ends where it best matches its start, and is crossfaded into itself there."""
    if flatten:
        env = envelope(y, rate, 30.0)
        i0 = int(t_att * rate)
        g = env[min(i0, len(env) - 1)] / np.maximum(env, 1e-5)
        g = np.clip(g, 0.0, 16.0)
        ramp = np.clip((np.arange(len(y)) - i0 * 0.6) / max(1.0, i0 * 0.4), 0, 1)     # from the natural attack ...
        y = y * (1 + (g - 1) * ramp)                                                   # ... into the level held
    period = rate / f0
    ls0 = (int(t_att * rate) // 32) * 32
    best, ls, le, xf = -2.0, ls0, 0, 0
    # the loop's start: a few blocks around the wanted time; its end: a whole number of periods later, where the
    # stretch before it best matches the stretch before the start
    for lsc in range(max(64, ls0 - 8 * 32), ls0 + 9 * 32, 32):
        x_ = int(min(lsc, max(3 * period, 0.03 * rate)))
        want = max(2, round(t_loop * rate / period)) * period
        if lsc + want + 2 * period >= len(y):
            want = max(2, int((len(y) - lsc - 2 * period) / period)) * period
        a_ = y[lsc - x_:lsc]
        na = np.dot(a_, a_)
        for nper in (-1, 0, 1):
            c0 = lsc + want + nper * period
            for cand in range(int(c0 - 0.5 * period), int(c0 + 0.5 * period) + 1):
                if cand > len(y) or cand - lsc < 2 * period:
                    continue
                b_ = y[cand - x_:cand]
                r = float(np.dot(a_, b_) / (np.sqrt(na * np.dot(b_, b_)) + 1e-12))
                if r > best:
                    best, ls, le, xf = r, lsc, cand, x_
    w = np.linspace(0, 1, xf)
    y = y.copy()
    y[le - xf:le] = y[le - xf:le] * (1 - w) + y[ls - xf:ls] * w
    return y[:le + 64], ls, le, best


def scan(folder, pattern):
    """files matching the regex -> list of paths"""
    rx = re.compile(pattern)
    return sorted(os.path.join(folder, f) for f in os.listdir(folder) if rx.fullmatch(f))


def pitched(bank, label, files, roots, rate, t_att, t_loop, scale, key_shift=0, gain=1.0, flatten=True, lo_hz=30.0,
            lp=None):
    """one zone a root: the source note nearest to it (its own pitch measured), tuned by the zone's rate factor"""
    cache, raw = {}, {}
    for p in files:
        x, fs = read_audio(p)
        x = trim_onset(x, fs)
        if len(x) < fs // 3:
            continue
        raw[p] = (x, fs)
    # the library's octave numbering: the median of (measured - written), from the notes high enough to measure
    # without octave errors
    diffs = []
    for p, (x, fs) in raw.items():
        nm = name_midi(p)
        f = detect_f0(x, fs, lo=lo_hz)
        if nm is not None and np.isfinite(f) and f > 90:
            diffs.append(69 + 12 * np.log2(f / 440.0) - nm)
    octave = 12 * int(round(np.median(diffs) / 12)) if diffs else 0
    for p, (x, fs) in raw.items():
        nm = name_midi(p)
        f = detect_f0(x, fs, near=midi_hz(nm + octave)) if nm is not None else detect_f0(x, fs, lo=lo_hz)
        if not np.isfinite(f) or f <= 0:
            continue
        m = 69 + 12 * np.log2(f / 440.0)
        cache[p] = (x, fs, f, m)
    print(f"  ({len(cache)} notes, octave numbering {octave:+d})")
    first = len(bank.zones)
    for i, root in enumerate(roots):
        p = min(cache, key=lambda q: abs(cache[q][3] - root))
        x, fs, f, m = cache[p]
        y = resample(x, fs, rate)
        if lp:                                          # (a speaker's roll-off: also what the codec predicts best)
            b_, a_ = signal.butter(2, lp / (rate / 2))
            y = signal.lfilter(b_, a_, y)
        y, ls, le, r = make_loop(y, rate, f, t_att * scale, t_loop * scale, flatten)
        hi = 127 if i == len(roots) - 1 else (root + roots[i + 1]) // 2
        # k comes from the source note's own measured pitch: any key the zone plays is then exact
        bank.add(f"{label} {root}", y, rate, loop=(ls, le), f0=f, root=root + key_shift, hi=hi + key_shift, gain=gain)
        z = bank.zones[-1]
        print(f"  {label:10s} root {root:3d} <- {os.path.basename(p):38s} {f:7.1f} Hz ({m:5.2f})  loop r={r:.2f}"
              f"  {z['bytes']:6d} B  snr {z['snr']:.0f} dB")
    return first, len(roots)


def drum(bank, label, parts, rate, dur, scale, gain=1.0, fade=0.35, hp=None):
    """parts: [(path, weight)] mixed (the mics of one hit); a one-shot of dur seconds, faded out over its last part"""
    mix, fs0 = None, None
    for p, wgt in parts:
        x, fs = read_audio(p)
        fs0 = fs
        if mix is None:
            mix = x * wgt
        else:
            n = max(len(mix), len(x))
            mix = np.pad(mix, (0, n - len(mix))) + np.pad(x, (0, n - len(x))) * wgt
    y = trim_onset(mix, fs0, rel=0.02, pre_ms=1.0)
    y = resample(y, fs0, rate)
    if hp:
        b, a = signal.butter(2, hp / (rate / 2), "high")
        y = signal.lfilter(b, a, y)
    n = min(len(y), int(dur * scale * rate))
    y = y[:n].copy()
    nf = int(n * fade)
    y[n - nf:] *= np.linspace(1, 0, nf) ** 2
    i = bank.add(label, y, rate, gain=gain)
    z = bank.zones[i]
    print(f"  {label:14s} {n / rate:5.2f} s @ {rate}  {z['bytes']:6d} B  snr {z['snr']:.0f} dB")
    return i


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scale", type=float, default=1.0)
    ap.add_argument("--wav", default=None)
    ap.add_argument("--out", default=OUT)
    a = ap.parse_args()
    os.makedirs(BUILD, exist_ok=True)
    if a.wav:
        os.makedirs(a.wav, exist_ok=True)
    subprocess.run(["cc", "-O2", "-I", os.path.join(HERE, "..", "firmware", "src"), "-o", CODEC,
                    os.path.join(HERE, "erkcodec.c"), "-lm"], check=True)
    bank = Bank(a.wav)
    S = a.scale
    inst = {}

    print("electric bass (Growlybass)")
    d = os.path.join(SRC, "growlybass", "Growlybass", "sustain")
    inst["EBASS"] = pitched(bank, "ebass", scan(d, r"[a-g]b?\d_f_rr1\.wav"), [28, 33, 38, 43, 48], 14700, 0.30, 0.22, S, lp=3200)

    print("upright bass (Meatbass)")
    d = os.path.join(SRC, "meatbass", "Meatbass", "Samples", "pizz")
    inst["UBASS"] = pitched(bank, "ubass", scan(d, r"[a-g]b?\d_vl3_rr1\.wav"), [28, 33, 38, 43, 48], 14700, 0.26, 0.20, S, lp=3200)

    print("tenor saxophone (VCSL)")
    d = os.path.join(SRC, "vcsl", "repo", "Aerophones", "Reed Aerophones", "Tenor Saxophone", "Non-Vibrato")
    inst["TENOR"] = pitched(bank, "tenor", scan(d, r"BrettTenor_NV_Main_[A-G]#?\d_vl3_rr1\.wav"), [47, 53, 59, 65, 71],
                            22050, 0.16, 0.24, S, flatten=False, lo_hz=70.0)

    print("hollowbody guitar (Black And Green Guitars, green)")
    d = os.path.join(SRC, "guitars", "Samples", "green", "ord")
    files = scan(d, r"twang_[a-g]b?\d_f_rr1\.wav")
    inst["GUITAR"] = pitched(bank, "guitar", files, [50, 56, 62, 68, 74], 22050, 0.22, 0.18, S, lo_hz=70.0)

    print("drums (Virtuosity Drums)")
    V = os.path.join(SRC, "virtuosity", "Samples")

    def mics(piece, name, w):
        """the same hit at several mics: {mic: weight}"""
        out = []
        for m, g in w.items():
            p = os.path.join(V, m, piece, f"{m}_{name}.flac")
            if not os.path.exists(p):
                p = os.path.join(V, m, piece, f"{m}_{name}_rr1.flac")
            out.append((p, g))
        return out

    dz = {}
    dz["KICK"] = drum(bank, "kick", mics("kick", "kick_snoff_vl3", {"kickmic": 1.0, "oh": 0.35}), 14700, 0.30, S)
    dz["SNARE"] = drum(bank, "snare", mics("snare", "snare_center_vl30", {"snaremic": 0.8, "oh": 0.7}), 22050, 0.34, S)
    dz["SNARE_SOFT"] = drum(bank, "snare soft", mics("snare", "snare_center_vl9", {"snaremic": 0.8, "oh": 0.7}), 22050, 0.20, S)
    dz["RIM"] = drum(bank, "cross-stick", mics("snare", "snare_crossstick_vl11", {"snaremic": 0.7, "oh": 0.8}), 22050, 0.13, S)
    dz["HAT"] = drum(bank, "hat closed", mics("hh", "hh_closed_vl3", {"oh": 1.0}), 29400, 0.10, S, hp=400)
    dz["HAT_OPEN"] = drum(bank, "hat open", mics("hh", "hh_open_vl3", {"oh": 1.0}), 29400, 0.40, S, hp=400)
    dz["RIDE"] = drum(bank, "ride", mics("ride", "ride_ride_vl2", {"oh": 1.0}), 29400, 0.75, S, fade=0.6, hp=300)
    dz["CRASH"] = drum(bank, "crash", mics("crash", "crash_crash_vl3", {"oh": 1.0}), 29400, 0.95, S, fade=0.7, hp=200)
    dz["TOM_HI"] = drum(bank, "tom high", mics("htom", "htom_center_vl12", {"oh": 1.0, "mid": 0.5}), 14700, 0.36, S)
    dz["TOM_LO"] = drum(bank, "tom low", mics("ltom", "ltom_center_vl12", {"oh": 1.0, "mid": 0.5}), 14700, 0.42, S)

    total = len(bank.data)
    print(f"bank: {len(bank.zones)} zones, {total} bytes ({total / 1024:.1f} KiB)")
    with open(a.out, "w") as f:
        f.write("/* Generated by tools/build_samples.py: do not edit. The samples are CC0 (LICENSING.md):\n"
                " * Karoryfer Growlybass, Meatbass and Black And Green Guitars, Versilian / Karoryfer Virtuosity Drums,\n"
                " * the Versilian Community Sample Library's tenor saxophone. */\n"
                "#ifndef SAMPLE_BANK_H\n#define SAMPLE_BANK_H\n#include <stdint.h>\n\n")
        for k, (first, n) in inst.items():
            f.write(f"#define SMP_{k} {first}\n#define SMP_{k}_N {n}\n")
        for k, i in dz.items():
            f.write(f"#define SMP_DR_{k} {i}\n")
        f.write(f"#define SMP_NZONES {len(bank.zones)}\n#define SMP_DATA_BYTES {total}\n\n")
        f.write("static const SmpZone SMP_ZONES[SMP_NZONES] = {\n")
        for z in bank.zones:
            f.write(f"    {{ {z['off']}u, {z['len']}u, {z['loop']}u, {z['loop_len']}u, {z['h1']}, {z['h2']}, {z['k']}u, "
                    f"{z['gain']}, {z['root']}, {z['hi']} }},   /* {z['name']}: {z['rate']} Hz, {z['bytes']} B */\n")
        f.write("};\n\nstatic const uint8_t SMP_DATA[SMP_DATA_BYTES] __attribute__((aligned(4))) = {\n")
        d = bytes(bank.data)
        for i in range(0, len(d), 24):
            f.write("    " + ",".join(str(b) for b in d[i:i + 24]) + ",\n")
        f.write("};\n#endif\n")
    print("->", os.path.relpath(a.out))


if __name__ == "__main__":
    sys.exit(main())
