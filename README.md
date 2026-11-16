# TexasBlues-1 (TB-1)

**Autonomous, Generative 12-Bar Blues Accompaniment Firmware for the M-VAVE FM-1 Synthesizer**

[![Firmware CI](https://github.com/provostprovost/blues/actions/workflows/build.yml/badge.svg)](https://github.com/provostprovost/blues/actions)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSING.md)
[![Hardware: M-VAVE FM-1](https://img.shields.io/badge/Hardware-M--VAVE%20FM--1-orange.svg)](https://github.com/zvenson/dinghy)

TexasBlues-1 (TB-1) turns the pocket-sized **M-VAVE FM-1** synthesizer into a standalone, autonomous rhythm section designed for solo guitarists practicing or performing without a keyboard or bass player. 

Built on the open-source `zvenson/dinghy` v0.5 framework, TB-1 generates a three-part blues band in real time with zero latency:
- **Driving Walking Electric Bass**: Melodic, target-note walking basslines with directional chromatic and diatonic approaches.
- **Guide-Tone Comping Engine**: Rootless Bill Evans and Ray Charles blues voicings across 4 sound engines (Rhodes FM, Bright FM EP, B3 Drawbar Organ, and Clavinet) sitting in a dedicated high comping register.
- **Acoustic Synthesized Drum Kit**: 4-voice transient drum machine featuring a pitch-swept kick, body+noise snare, metallic LFSR hi-hat/ride, and rimshot metronome.

Everything runs entirely on the internal 240 MHz RISC microcontroller using pure 32-bit fixed-point integer DSP with zero floating-point overhead.

---

## Table of Contents

- [Quick Start & Installation](#quick-start--installation)
- [Hardware Controls & Interface](#hardware-controls--interface)
  - [Rotary Encoders](#rotary-encoders)
  - [14 Hardware Buttons](#14-hardware-buttons)
  - [27 Backlit Silicone Keys](#27-backlit-silicone-keys)
- [Display Views](#display-views)
- [Accompaniment Engine Features](#accompaniment-engine-features)
  - [Groove Styles](#5-groove-styles)
  - [Sound Engines (Keys Algorithms)](#4-keys-sound-engines)
  - [The "Spice" Macro](#the-spice-macro)
  - [Key Decoupling & Preset System](#key-decoupling--preset-system)
- [External MIDI & Footswitch Control](#external-midi--footswitch-control)
- [Building from Source](#building-from-source)
- [Recovery & Restoring Stock Firmware](#recovery--restoring-stock-firmware)
- [License](#license)

---

## Quick Start & Installation

### Requirements
- **Hardware**: M-VAVE FM-1 Pocket Synthesizer
- **Cable**: USB-C data cable connected to your computer
- **Python**: Python 3.8+ with `mido` and `python-rtmidi` (`pip install mido python-rtmidi`)

### Installing Prebuilt Firmware
1. Connect your M-VAVE FM-1 to your computer via USB and turn it on.
2. Clone this repository:
   ```bash
   git clone https://github.com/provostprovost/blues.git
   cd blues
   ```
3. Flash the firmware package directly to your device:
   ```bash
   python3 tools/fm1_install.py build/dinghy.fwsc --yes
   ```
4. The FM-1 screen will show an update progress bar, restart, and boot directly into TexasBlues-1!

---

## Hardware Controls & Interface

```
+-------------------------------------------------------------+
|   [SELECT]          [ALGORITHM]           [PRESETS]         |
|  (Groove Style)   (Keys Sound Engine)   (Preset / Key)      |
|                                                             |
|       +---------------------------------------------+       |
|       |       240x240 RGB565 COLOR TFT LCD          |       |
|       |  • 12-Bar Lead Sheet with Beat Pips         |       |
|       |  • 8-Channel Mixer with Level Meters        |       |
|       +---------------------------------------------+       |
|                                                             |
|   [KNOB 1]          [KNOB 2]          [KNOB 3]     [KNOB 4] |
| (BPM / Swing)     (Spice / Tune)    (Bass / Tone) (Keys/Rev)|
|                                                             |
| [PLAY] [REC] [OCT-] [OCT+] [HOME] [SAVE] [GLO] [ARP] [FX]   |
| [EDIT] [ENV] [SEQ]  [LFO]  [SEL]                            |
|                                                             |
|  [F3] [F#3] [G3] [Ab3] [A3] [Bb3] [B3] [C4] [C#4] [D4]...   |
|  (27 White Backlit Silicone Keys with Beat-1 Pulse)         |
+-------------------------------------------------------------+
```

### Rotary Encoders

| Encoder | Label | Primary Function (Turn) | Secondary Function (Hold `GLO` + Turn) |
|---|---|---|---|
| **Encoder 0** | `SELECT` | **Groove Style**: Cycles through the 5 groove styles | — |
| **Encoder 1** | `ALGORITHM` | **Keys Engine**: Rhodes FM, Bright EP, B3 Organ, Clavinet | — |
| **Encoder 2** | `KNOB 1` | **Tempo**: Adjusts BPM ($40\text{--}240\text{ BPM}$) | **Swing %**: Adjusts shuffle swing ($50\%\text{--}80\%$) |
| **Encoder 3** | `KNOB 2` | **Spice Level**: Adjusts harmonic/rhythmic complexity ($0\%\text{--}100\%$) | **Master Tuning**: $440\text{ Hz}$, $432\text{ Hz}$, or $442\text{ Hz}$ |
| **Encoder 4** | `KNOB 3` | **Bass Volume**: Adjusts walking bass level | **Bass Tone**: Low-pass cutoff filter ($30\%\text{--}100\%$) |
| **Encoder 5** | `KNOB 4` | **Keys Volume**: Adjusts comping chord level | **Reverb Mix**: Schroeder room reverb wet level |
| **Encoder 6** | `PRESETS` | **Preset Slot**: Selects Presets 0--15 (preserves key) | **Key Select** (Hold `SEL`): Scrolls chromatic keys |

---

### 14 Hardware Buttons

All 14 front-panel buttons have dedicated functions:

| Button | Label | Live Musical Function |
|---|---|---|
| **`PLAY`** | Transport | **Start / Stop**: Toggles playback (Green LED illuminates when playing). |
| **`REC`** | Metronome | **Count-in Toggle**: Enables 1-bar count-in rimshot click. Pulses on Beat 1. |
| **`OCT-`** | Tempo Down | Decrements tempo by **-4 BPM**. |
| **`OCT+`** | Tempo Up | Increments tempo by **+4 BPM**. |
| **`HOME`** | Display | **Screen Toggle**: Switches between **12-Bar Lead Sheet** and **Band Mixer**. |
| **`SAVE`** | Storage | **Commit to Flash**: Saves current preset and parameters immediately to flash. |
| **`GLO`** | Shift | **Shift Layer**: Hold down to access secondary functions on Knobs 1--4. |
| **`ARP`** | Tap Tempo | **Tap Tempo**: Tap repeatedly in rhythm to automatically calculate and set BPM. |
| **`FX`** | Accent Stab | **Manual Accent Stab**: Triggers an immediate full-band accent stab and crash. |
| **`EDIT`** | Turnaround | **Early Turnaround Cue**: Bar-quantized jump straight to Measure 9 (V chord). |
| **`ENV`** | Vamp | **Vamp Toggle**: Freezes the 12-bar form and vamps indefinitely on the current chord. |
| **`SEQ`** | Stop-Time | **Stop-Time Break**: Triggers a 4-bar stop-time break with beat-1 downbeat stabs. |
| **`LFO`** | Form Mode | **Minor Blues Toggle**: Instantly toggles between 12-Bar Major and Minor Blues. |
| **`SEL`** | Key Select | **Key Select Modifier**: Hold `SEL` and turn `PRESETS` to scroll through all 12 keys. |

---

### 27 Backlit Silicone Keys

The silicone keyboard is fully backlit with individual white LEDs. The LED under the active root key **pulses as a visual heartbeat metronome on Beat 1** of every measure.

- **Keys 0 to 11 (Chromatic Key Select)**:
  - Tapping any key immediately sets the jam key:
    `Key 0 = F`, `Key 1 = F#`, `Key 2 = G`, `Key 3 = Ab`, `Key 4 = A`, `Key 5 = Bb`, `Key 6 = B`, `Key 7 = C`, `Key 8 = C#`, `Key 9 = D`, `Key 10 = Eb`, `Key 11 = E`.
  - **Single Tap while playing**: Queues key modulation seamlessly at the start of the next 12-bar chorus.
  - **Double Tap (< 250 ms) while playing**: Executes an immediate in-bar key modulation.
- **Keys 12 to 17 (Live Performance Cues)**:
  - Key 12: Return to Standard 12-Bar Form
  - Key 13: Vamp Toggle
  - Key 14: 4-Bar Stop-Time Break
  - Key 15: Chord Hold
  - Key 16: Immediate Turnaround (Jump to Bar 9)
  - Key 17: Manual Accent Stab
- **Keys 18 to 22 (Form Variations)**:
  - Standard 12-Bar, Quick-Change (IV in Bar 2), 1-4-5 Blues, 8-Bar Blues, Minor Blues.
- **Keys 23 to 26 (Outro Cadences)**:
  - Armed outro triggers: Immediate Stop, Tag Ending, Triple Turnaround, Chromatic Slide Outro.

---

## Display Views

Press **`HOME`** to toggle between the two non-blocking 240x240 RGB TFT screens:

### 1. 12-Bar Lead Sheet View
- Displays the complete 12-bar chord grid with large, easy-to-read chord symbols (e.g. `E7`, `A7`, `B7`).
- **Differential Tile Rendering**: Zero screen flicker. The active measure box highlights in amber, and 4 beat pips track the groove in real time.
- Status bar displays: `KEY: E`, `TEMPO: 112`, `STYLE: TEXAS SHUFFLE`, `SPICE: 65%`, and active cues (`VAMP`, `TURNAROUND`, `STOP-TIME`).

### 2. Band Mixer View
- 8-channel virtual console with live visual meters:
  1. `BASS`: Walking Bass Volume & Cutoff Filter
  2. `KEYS`: Comping Voicing Volume
  3. `KICK`: Bass Drum Level
  4. `SNAR`: Snare Drum Level
  5. `HAT`: Hi-Hat / Ride Cymbal Level
  6. `RIM`: Rimshot Click Level
  7. `REV`: Schroeder Room Reverb Mix
  8. `DRV`: Analog Tube Soft-Clip Drive Level

---

## Accompaniment Engine Features

### 5 Groove Styles

Selected with **Encoder 0 (`SELECT`)**:

1. **Texas Shuffle**: Stevie Ray Vaughan / Double Trouble driving blues shuffle with tight snare backbeat, walking boogie bass, and syncopated off-beat comping (Default 67% swing, 112 BPM).
2. **Slow 12/8 Blues**: B.B. King / Buddy Guy late-night 12/8 slow blues ballad with rolling triplet ride cymbals, expressive piano chords, and deep bass pedal notes (68 BPM).
3. **Chicago Swing**: Muddy Waters / Little Walter uptempo swinging blues with walking quarter-note bass, crisp hi-hat chicks, and Evans guide-tone comping (130 BPM).
4. **Rockin' 8s**: Chuck Berry / Hound Dog straight-eighth rock 'n' roll boogie with driving eighth-note basslines and biting clavinet stabs (140 BPM).
5. **2nd Line Funk**: New Orleans syncopated street beat with polyrhythmic snare accents, bouncing basslines, and syncopated rhythmic stabs (96 BPM).

---

### 4 Keys Sound Engines

Selected with **Encoder 1 (`ALGORITHM`)**:

1. **Rhodes FM (Algo 0)**: 2-Operator FM synthesis producing a sweet bell/tine attack that sustains into a warm, mellow electric piano fundamental.
2. **Bright FM EP (Algo 1)**: 2:1 FM chime algorithm with crisp attack transients and rich upper harmonics for cutting through dense mixes.
3. **B3 Drawbar Organ (Algo 2)**: 4-harmonic additive Hammond B3 emulation (`888800000` drawbar registration) with cutting upper harmonic drawbars for classic gospel/blues comping.
4. **Funk Clavinet (Algo 3)**: Biting 3:1 plucked FM algorithm with fast decay, ideal for percussive rhythm chops and syncopated funk lines.

---

### The "Spice" Macro

Controlled via **Encoder 3 (`KNOB 2`)** from $0\%$ to $100\%$:

- **$0\%\text{--}30\%$ (Straight & Clean)**: 2-note guide-tone shells (3rd + 7th), simple root-fifth walking bass, and straight groove timing.
- **$31\%\text{--}65\%$ (Authentic Blues Comping)**: 3-note and 4-note Evans voicings (adding 9ths and 13ths), directional beat-4 chromatic approaches on the bass, and dynamic velocity humanization.
- **$66\%\text{--}100\%$ (Bebop & Modern Blues)**: Lush altered voicings, passing diminished chords, tritone substitutions, bass octave pops, and syncopated off-beat comping stabs.

---

### Key Decoupling & Preset System

- **Key Persistence**: The active musical key is **strictly decoupled from preset switching**. You can jam in the key of **E**, browse through all 16 presets (switching between Texas Shuffle, Slow 12/8, Chicago Swing, and Funk), and every preset will immediately play in **E** without interrupting your flow.
- **Presets 0 to 15**: Full accompaniment snapshots (groove style, tempo, swing %, EP algorithm, spice level, and mixer balances).
- **Instant Save**: Press **`SAVE`** at any time to commit your current settings to the FM-1's non-volatile onboard flash storage.

---

## External MIDI & Footswitch Control

Connect any USB-MIDI or 3.5mm TRS Type-A MIDI footswitch controller to operate the accompaniment hands-free while playing guitar:

### MIDI Continuous Controllers (CC)
| MIDI CC | Value Range | Mapped Function |
|---|---|---|
| **CC 64** (Sustain) | $\ge 64$ | **Vamp Mode Toggle**: Freeze on active chord |
| **CC 80** | $\ge 64$ | **Early Turnaround**: Jump immediately to Measure 9 |
| **CC 81** | $\ge 64$ | **Stop-Time Break**: Trigger 4-bar break with beat-1 stabs |
| **CC 82** | $\ge 64$ | **Next Key**: Increment root key chromatically (+1 semitone) |
| **CC 83** | $\ge 64$ | **Manual Accent Stab**: Trigger full-band accent crash |

### MIDI Clock Synchronization
- **Master Mode**: Transmits MIDI Clock (`0xF8`), Start (`0xFA`), and Stop (`0xFC`) to sync external drum machines, delays, and loopers via USB-MIDI OUT.
- **Slave Mode**: Connect an external drum machine or DAW to TRS MIDI IN; TB-1 automatically locks its tempo and transport to the incoming MIDI clock stream.

---

## Building from Source

To compile the firmware yourself:

### 1. Prerequisites
- Docker (macOS or Windows) OR Linux x86-64 with the JieLi toolchain installed.
- Python 3 with `Pillow`, `mido`, and `python-rtmidi`.

### 2. Run the Unit Test Suite
Verify all harmonic, voice-leading, DSP, and panel logic on your local machine:
```bash
gcc -DSTANDALONE_TEST -O2 -Wall -Wextra -Werror -Ifirmware/src \
    firmware/src/blues_harmony.c firmware/src/blues_arranger.c \
    firmware/src/dsp_voices.c firmware/src/panel_input.c \
    firmware/src/ui_screens.c tests/test_blues_engine.c \
    -o build/test_blues && ./build/test_blues
```

### 3. Build the Firmware Package
Run the build script:
```bash
./build.sh
```
The output package will be generated at `build/dinghy.fwsc`.

---

## Recovery & Restoring Stock Firmware

The M-VAVE FM-1 features a hardware-level bootloader that cannot be bricked:

- **USB Rescue Mode**: Hold **`OCT-`** while turning on the FM-1 power switch. The device will boot directly into the USB updater without loading the application.
- **Reflashing**: Run `python3 tools/fm1_install.py build/dinghy.fwsc --yes` to reinstall TexasBlues-1.
- **Restoring Factory Firmware**: Upload the original stock M-VAVE firmware file via `tools/fm1_install.py <stock_firmware.bin>` or through the [FM-1 Firmware Hub](https://thefirmwarehub.com).

---

## License

TexasBlues-1 is free, open-source software distributed under the **GNU General Public License v3.0 (GPL-3.0)**. See [LICENSING.md](LICENSING.md) for details.

Built on the [Dinghy framework](https://github.com/zvenson/dinghy) by zvenson, derived from [Felucca](https://github.com/hugelton/Felucca) by Leo Kuroshita (Hügelton Instruments) and [SLOOP](https://github.com/isod89/sloop-fm1) by isod89.
