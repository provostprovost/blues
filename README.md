# TexasBlues-1 (TB-1)

**A standalone 12-bar blues backing band firmware for the M-VAVE FM-1 pocket synthesizer.**

[![Firmware CI](https://github.com/provostprovost/blues/actions/workflows/build.yml/badge.svg)](https://github.com/provostprovost/blues/actions)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSING.md)

Turn your **M-VAVE FM-1** into an always-ready blues rhythm section—drums, walking bass, and comping chords—that follows a real 12-bar form while you solo on guitar or play the built-in 4-voice lead keyboard.

- **Real blues accompaniment:** 8 groove styles, 8 bass engines, 12 chord engines, and 8 drum kits—plus independent bass, keys, and drum rhythm styles.
- **Playable lead synth:** 8 solo voices across 27 backlit keys with the blues scale lit up under your fingers.
- **Live jam controls:** Toggle major/minor blues, trigger stop-time breaks, vamp on a chord, or cue a turnaround on the fly.
- **Save your setup:** Dial in your favorite sound and hit `SAVE` so it boots right back into your jam.

---

## Quick Start (60 Seconds)

1. Install Python dependencies:
   ```bash
   pip install mido python-rtmidi
   ```
2. Plug the FM-1 into your computer with a USB-C data cable and power it on.
3. Flash the firmware:
   ```bash
   git clone https://github.com/provostprovost/blues.git
   cd blues
   python3 tools/fm1_install.py build/dinghy.fwsc --yes
   ```
4. Once the FM-1 reboots, press **`PLAY/STOP`** to start the band.
5. Turn **`SELECT`** to change the groove, **`KNOB1`** for tempo, **`ALGORITHM`** for chords, and **`PRESETS`** for the lead keyboard sound.

---

## Panel Layout

```text
+-------------------------------------------------------------------+
| [MASTER]  [SELECT]   +---------------+   [KNOB1] [KNOB2] [KNOB3] [KNOB4]
|                      |  240x240 LCD  |                            |
| [PRESETS] [ALGORITHM]|  Lead Sheet / |   [FX]  [SEL] [ENV] [LFO] [EDIT] [GLO]
|                      |  Band Mixer   |   [HOME][SAVE][ARP] [SEQ] [PLAY] [REC]
|      [OCT-] [OCT+]   +---------------+                            |
|                                                                   |
|  [F] [F#] [G] [Ab] [A] [Bb] [B] [C] [C#] [D] [Eb] [E] ... (27 keys)
+-------------------------------------------------------------------+
```

---

## Controls Cheat Sheet

### Knobs

| Knob | Normal Turn | Hold `GLO` + Turn | Hold `SEL` + Turn |
|---|---|---|---|
| **`MASTER`** | Hardware master volume | — | — |
| **`SELECT`** | Groove Style (8 styles) | Drum Kit (8 kits) | Blues Form (Major, Quick-Change, 1-4-5, 8-Bar, Minor) |
| **`PRESETS`** | Solo Lead Voice (8 voices) | Key (`C`–`B`) | Key (`C`–`B`) |
| **`ALGORITHM`** | Backing Chord Engine (12 engines) | — | Keys Comping Style (Charleston, Backbeat, Call & Resp, Pad, Chop) |
| **`KNOB1`** | Tempo (40–240 BPM)<br>*(Mixer: Bass Volume)* | Swing (0%–100%)<br>*(Mixer: Reverb Mix)* | Drum Beat Pattern (8 patterns) |
| **`KNOB2`** | Spice (0%–100%)<br>*(Mixer: Keys Volume)* | Master Tuning (440 / 432 / 442 Hz)<br>*(Mixer: Spice)* | Fullness (0%–100%, sparse vs. rocking) |
| **`KNOB3`** | Bass Volume<br>*(Mixer: Drums Volume)* | Bass Sound (8 basses)<br>*(Mixer: Swing)* | Bass Line Style (Walking, Shuffle, Boogie, Root-5th, Funk) |
| **`KNOB4`** | Keys Volume<br>*(Mixer: Lead Volume)* | Reverb Mix<br>*(Mixer: Fullness 0%–100%)* | — |

### Buttons

| Button | What It Does |
|---|---|
| **`PLAY/STOP`** | Start or stop playback (LED lights up while playing). |
| **`REC`** | Toggle 1-bar count-in rimshot click (pulses on Beat 1). |
| **`OCT-` / `OCT+`** | Shift the 27-key solo keyboard down or up (-2 to +2 octaves). Hold `SEL` + `OCT-`/`OCT+` to nudge tempo ±4 BPM. |
| **`HOME`** | Switch between the **12-Bar Lead Sheet** and **Band Mixer** screens. |
| **`SAVE`** | Save your current groove, tempo, key, voices, and mixer levels to flash. |
| **`GLO`** | Shift key for secondary knob functions (Swing, Reverb, Drum Kit, Bass Engine, Tuning). |
| **`ARP`** | Tap Tempo—tap in rhythm to set the BPM. |
| **`FX`** | Trigger a manual full-band accent stab + crash. |
| **`EDIT`** | Early Turnaround—jump straight to Bar 9 (the V chord) at the next bar. |
| **`ENV`** | Vamp Mode—freeze the progression and loop the current chord. |
| **`SEQ`** | Trigger a 4-bar stop-time break with beat-1 stabs. |
| **`LFO`** | Toggle **Major 12-Bar** ↔ **Minor Blues**. |
| **`SEL`** | Shift key for `SELECT` (form), `PRESETS` (key), `OCT-`/`OCT+` (tempo), and silicone key shortcuts. |

---

## "How Do I…" Quick Recipes

- **Change the key:** Hold **`SEL`** (or **`GLO`**) and turn **`PRESETS`**. Or hold **`SEL`** and tap one of the first 12 silicone keys (`F` through `E`): a single tap queues the key change for the next chorus, and a quick double tap switches right away.
- **Switch between Major and Minor Blues:** Press **`LFO`** for a one-tap toggle. You can also hold **`SEL`** and turn **`SELECT`**, or hold **`SEL`** and press Key 22 (Minor) / Key 18 (Major). Minor Blues shows **`KEY:Em (MIN)`** in cyan at the top of the screen and updates all 12 chord boxes (`Em7`, `Am7`, `C7`, `B7`).
- **Make the chords and bass jazzier:** Turn **`KNOB2`** (**Spice**). Low (0–30%) keeps it straight and simple; middle (31–65%) adds 9ths, 13ths, and chromatic bass walk-ups; high (66–100%) adds passing diminished chords, tritone subs, and turnaround chords in Bars 11–12.
- **Dial in sparse vs. full-band energy:** On the Band Mixer (`HOME`), hold **`GLO`** and turn **`KNOB4`** (**Fullness**), or on the Lead Sheet hold **`SEL`** and turn **`KNOB2`**. Low (<35%) strips the band down to a thin, intimate trio (half-note bass, soft backbeat chords, light rimshot/hat); high (>70%) adds driving bass anticipations, push comping stabs, ghost snares, and punchy Bar 4/8/12 drum fills.
- **Adjust shuffle feel:** Hold **`GLO`** and turn **`KNOB1`** (**Swing**), or press **`HOME`** to open the Band Mixer and hold **`GLO`** + turn **`KNOB3`**. 0% is straight 8ths, ~67% is a classic triplet shuffle, and 100% is a hard dotted-8th swing.
- **Play along on the keys:** Just play the 27 silicone keys—they act as a 4-voice polyphonic lead synth with automatic finger vibrato on held notes. While playing, the LEDs always stay at a clean **6 notes per octave**: **Minor Blues** (`1, b3, 4, b5, 5, b7`) in Minor Blues (the default startup mode in E Minor); in Major forms, **Major Blues** (`1, 2, b3, 3, 5, 6`) on `I7` bars and **Minor Blues** (`1, b3, 4, b5, 5, b7`) on `IV7`/`V7` bars, with a quick Beat-1 flash on the root.
- **Arm an ending:** Hold **`SEL`** and tap Keys 23–26 (23: Hard Stop, 24: Tag Ending, 25: Triple Turnaround, 26: Chromatic Slide).
- **Save your setup:** Press **`SAVE`**. Background autosave is intentionally off so temporary jam tweaks don't overwrite your saved setup.

<details>
<summary>All <code>SEL</code> + Silicone Key Shortcuts (Keys 0–26)</summary>

- **Keys 0–11 (Key Select):** `0=F`, `1=F#`, `2=G`, `3=Ab`, `4=A`, `5=Bb`, `6=B`, `7=C`, `8=C#`, `9=D`, `10=Eb`, `11=E` (single tap = next chorus, double tap = immediate)
- **Keys 12–17 (Live Cues):** `12` Return to 12-Bar, `13` Vamp Toggle, `14` 4-Bar Stop-Time, `15` Hold Chord, `16` Jump to Turnaround, `17` Accent Stab
- **Keys 18–22 (Form Variations):** `18` Standard 12-Bar, `19` Quick-Change (IV in Bar 2), `20` 1-4-5 Opening, `21` 8-Bar Blues, `22` Minor Blues
- **Keys 23–26 (Outros):** `23` Stop, `24` Tag Ending, `25` Triple Turnaround, `26` Chromatic Slide

</details>

---

## The Sounds

- **8 Groove Styles (`SELECT`):** 12/8 Slow Blues, Texas Shuffle, Chicago Swing, Rockin' Blues, New Orleans 2nd Line, Memphis Soul, Rhumba Blues (3-3-2), Boogie-Woogie.
- **12 Chord Engines (`ALGORITHM`):** Rhodes FM, Bright FM EP, Wurlitzer 200A, Clean B3 Organ, Dirty Gospel B3, Clavinet D6, Archtop Rhythm Guitar, Blues Horns, Honky-Tonk Piano, Tremolo Suitcase EP, Delta Resonator Guitar, Rotary Leslie Organ.
- **8 Solo Lead Voices (`PRESETS`):** Overdriven Blues Guitar, Chicago Blues Harp, Tenor Sax, Moog Lead, Warm Jazz Hollowbody ("Lucille"), Fuzz Octave, Muted Blues Trumpet, Percussive B3 Organ.
- **8 Bass Engines (`GLO` + `KNOB3`):** Fender P-Bass, Upright Double Bass, Muted Tic-Tac Boogie, Moog Funk Synth, 60s Jazz Bass, Flatwound Motown, Ampeg Tube Growl, Sub Octave.
- **8 Drum Kits (`GLO` + `SELECT`):** Studio Punch, Vintage Brush, Retro 808, Stax Tight, Chicago Room, Dry Vintage, Jazz Club, Heavy Blues-Rock.

---

## Display Screens

Press **`HOME`** to flip between the two screens:

1. **12-Bar Lead Sheet:** Shows the groove and drum kit at the top, Key + Form (`(MAJ)` or cyan `(MIN)`), BPM, and Spice below it, the 12-bar chord grid with the active bar highlighted in amber, 4 beat pips, live playback status, and the footer (`<Solo Lead> * <Chord Engine>`, aligned left-to-right with `PRESETS` and `ALGORITHM`).
2. **Band Mixer:** Live level bars for `BASS VOL`, `KEYS VOL`, `DRUMS VOL`, `LEAD VOL`, `REVERB`, `SPICE`, `SWING` (0%–100%), and `FULLNESS` (0%–100%).

---

## Building from Source & Tests

Requires Docker (macOS/Windows) or Linux x86-64 with the JieLi toolchain, plus Python 3 (`Pillow`, `mido`, `python-rtmidi`).

```bash
# Run the local C unit test suite
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Werror -Ifirmware/src \
    firmware/src/blues_harmony.c firmware/src/blues_arranger.c \
    firmware/src/dsp_voices.c firmware/src/panel_input.c \
    firmware/src/ui_screens.c tests/test_blues_engine.c \
    -o build/test_blues && ./build/test_blues

# Build the firmware package (outputs build/dinghy.fwsc)
./build.sh
```

---

## Recovery & Restoring Stock Firmware

The FM-1 bootloader cannot be bricked:
- **USB Rescue Mode:** Hold **`OCT-`** while turning on the power switch to boot straight into the USB updater.
- **Reflash TB-1:** `python3 tools/fm1_install.py build/dinghy.fwsc --yes`
- **Restore Stock Firmware:** Use M-Vave's updater with your factory firmware package, or flash a backup package with `tools/fm1_install.py`.

---

## License & Credits

Distributed under the **GNU General Public License v3.0 (GPL-3.0)**. See [LICENSING.md](LICENSING.md) for full per-file attribution and third-party licenses.

Built on [Dinghy](https://github.com/zvenson/dinghy) by **zvenson**, which stands on the work of **Leo Kuroshita (@kurogedelic, Hügelton Instruments)** ([Felucca](https://github.com/hugelton/Felucca)), **isod89** ([SLOOP](https://github.com/isod89/sloop-fm1)), **M. Geisler (genkigenki / makkuro)** ([genkigenki/sloop-fm1](https://github.com/genkigenki/sloop-fm1) — installer, packer, and loader), **illuminus5** ([Skiff](https://github.com/illuminus5/skiff) — update loader screen), and **ChanceTheMaker / keremimo** (Felucca Salt).
