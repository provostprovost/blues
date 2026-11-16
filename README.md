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
| (Groove / Drum)   (Chord Engine)        (Lead / Key)        |
|                                                             |
|       +---------------------------------------------+       |
|       |       240x240 RGB565 COLOR TFT LCD          |       |
|       |  • 12-Bar Lead Sheet with Beat Pips         |       |
|       |  • 7-Channel Band Mixer with Level Meters   |       |
|       +---------------------------------------------+       |
|                                                             |
|   [KNOB 1]          [KNOB 2]          [KNOB 3]     [KNOB 4] |
| (BPM / Bass)      (Spice / Keys)    (Bass / Drums)(Keys / Lead)|
|                                                             |
| [PLAY] [REC] [OCT-] [OCT+] [HOME] [SAVE] [GLO] [ARP] [FX]   |
| [EDIT] [ENV] [SEQ]  [LFO]  [SEL]                            |
|                                                             |
|  [F4] [F#4] [G4] [Ab4] [A4] [Bb4] [B4] [C5] [C#5] [D5]...   |
|  (27 White Backlit Silicone Keys with Blues Scale & Pulse)  |
+-------------------------------------------------------------+
```

### Rotary Encoders

The 4 lower knobs dynamically re-map when switching between **Lead Sheet View** and **Band Mixer View** (`HOME` button):

#### On Lead Sheet View (Default)
| Encoder | Label | Primary Function (Turn) | Secondary Function (Hold `GLO` + Turn) |
|---|---|---|---|
| **Encoder 0** | `SELECT` | **Groove Style**: Cycles 8 groove styles | **Drum Kit**: Studio, Brush, Retro 808, Stax Tight |
| **Encoder 1** | `ALGORITHM` | **Chord Engine**: 12 electric piano, organ, acoustic piano, guitar & horn engines | — |
| **Encoder 2** | `KNOB 1` | **Tempo**: Adjusts BPM ($40\text{--}240\text{ BPM}$) | **Swing %**: Adjusts shuffle swing ($0\%\text{--}100\%$) |
| **Encoder 3** | `KNOB 2` | **Spice Level**: Adjusts harmonic complexity ($0\%\text{--}100\%$) | **Master Tuning**: $440\text{ Hz}$, $432\text{ Hz}$, or $442\text{ Hz}$ |
| **Encoder 4** | `KNOB 3` | **Bass Volume**: Adjusts walking bass level | **Bass Engine**: P-Bass, Upright, Muted Boogie, Moog Funk |
| **Encoder 5** | `KNOB 4` | **Keys Volume**: Adjusts comping chord level | **Reverb Mix**: Schroeder room reverb wet level |
| **Encoder 6** | `PRESETS` | **Lead Solo Instrument**: 8 expressive guitar, harp, horn, moog & organ voices | **Key Select** (Hold `SEL` or `GLO`): Scrolls keys `C..B` |

#### On Band Mixer View (`HOME` Pressed)
On the Band Mixer view, Knobs 1--4 act as dedicated hardware mixing faders:
| Encoder | Label | Direct Turn (Fader) | Hold `GLO` + Turn |
|---|---|---|---|
| **Encoder 2** | `KNOB 1` | **Bass Level**: Adjusts `BASS VOL` | **Reverb Mix**: Schroeder room reverb level |
| **Encoder 3** | `KNOB 2` | **Keys Level**: Adjusts `KEYS VOL` | **Spice Level**: Harmonic complexity ($0\%\text{--}100\%$) |
| **Encoder 4** | `KNOB 3` | **Drums Level**: Adjusts `DRUMS VOL` | **Swing %**: Shuffle swing ($0\%\text{--}100\%$) |
| **Encoder 5** | `KNOB 4` | **Lead Level**: Adjusts `LEAD VOL` | **Master Tuning**: $440\text{ Hz}$, $432\text{ Hz}$, $442\text{ Hz}$ |

---

### 14 Hardware Buttons

All 14 front-panel buttons have dedicated functions:

| Button | Label | Live Musical Function |
|---|---|---|
| **`PLAY`** | Transport | **Start / Stop**: Toggles playback (Green LED illuminates when playing). |
| **`REC`** | Metronome | **Count-in Toggle**: Enables 1-bar count-in rimshot click. Pulses on Beat 1. |
| **`OCT-`** | Octave Down | **Keyboard Octave Down**: Shifts solo keyboard down (-2 to +2 octaves). Hold `SEL` + `OCT-` to nudge tempo down (-4 BPM). |
| **`OCT+`** | Octave Up | **Keyboard Octave Up**: Shifts solo keyboard up (-2 to +2 octaves). Hold `SEL` + `OCT+` to nudge tempo up (+4 BPM). |
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

### 27 Backlit Silicone Keys & Live Polyphonic Keyboard

The 27 silicone keys serve dual roles: a **live 4-voice polyphonic keyboard** during playback, and a **section & key navigation matrix** when holding `SEL`.

#### 1. Live Playable Polyphonic Keyboard (Without `SEL`)
- **4-Voice Polyphony & Chords**: Pressing multiple keys simultaneously plays rich multi-note chords or harmonized lead licks across 4 dynamic DSP voices with quietest-voice stealing.
- **Independent Note Tracking**: Holding a low drone or bass note while tapping and releasing higher notes releases only the tapped notes—the low note sustains cleanly without getting stuck.
- **Expressive Blues Vibrato**: Sustained notes naturally bloom into an authentic $\pm 27\text{-cent}$ finger vibrato after ~40 ms of gate hold.
- **2-Octave Lead Register ($F_4\text{--}G_6$, MIDI 65--91)**: Expressive soloing with touch velocity and instant response.
- **Full Backlight When Stopped / In Setup**: When playback is stopped or `SEL` is held, all 27 keys illuminate with full white backlighting so you can easily read printed labels in dim lighting.
- **Minor Blues Scale LED Guide (While Playing)**: During playback, LEDs illuminate **strictly under notes of the Minor Blues Scale** (`1, b3, 4, b5, 5, b7` relative to the active root key), providing an instant visual fretboard for soloing. The root note pulses on Beat 1 of every measure for rock-solid visual timing.

#### 2. Section Navigation & Form Controls (Hold `SEL`)
> **Note**: Pressing silicone keys **never** changes the session root key unless `SEL` is actively held down. This prevents accidental key modulations while auditioning or jamming.
- **Keys 0 to 11 (Chromatic Key Select)**:
  - `Key 0 = F`, `Key 1 = F#`, `Key 2 = G`, `Key 3 = Ab`, `Key 4 = A`, `Key 5 = Bb`, `Key 6 = B`, `Key 7 = C`, `Key 8 = C#`, `Key 9 = D`, `Key 10 = Eb`, `Key 11 = E`.
  - **Single Tap while playing**: Queues key modulation seamlessly at the start of the next 12-bar chorus.
  - **Double Tap (< 250 ms) while playing**: Executes an immediate in-bar key modulation.
- **Keys 12 to 17 (Live Performance Cues)**:
  - Key 12: Return to Standard 12-Bar Form
  - Key 13: Vamp Toggle (freeze on current chord)
  - Key 14: 4-Bar Stop-Time Break
  - Key 15: Chord Hold
  - Key 16: Immediate Turnaround (Jump to Bar 9)
  - Key 17: Manual Accent Stab
- **Keys 18 to 22 (Form Variations)**:
  - Standard 12-Bar, Quick-Change (IV in Bar 2), 1-4-5 Blues, 8-Bar Blues, Minor Blues.
- **Keys 23 to 26 (Outro Cadences)**:
  - Immediate Stop, Tag Ending, Triple Turnaround, Chromatic Slide Outro.

---

## Display Views

Press **`HOME`** to toggle between the two non-blocking 240x240 RGB TFT screens:

### 1. 12-Bar Lead Sheet View
- Displays the complete 12-bar chord grid with large, easy-to-read chord symbols (e.g. `E7`, `A7`, `B7`).
- **Differential Tile Rendering**: Zero screen flicker. The active measure box highlights in amber, and 4 beat pips track the groove in real time.
- **Header**: Active Groove Style & Drum Kit (e.g. `TX SHUFFLE [STUDIO]`), Session Key, BPM, and Spice %.
- **Footer**: Live Status (`PLAYING`, `VAMP LOOP ACTIVE`, `STOP-TIME BREAK`) and Sound Engines (`RHODES FM * BLUES GTR`).

### 2. Band Mixer View
- Live visual meters for all instrument levels and macro parameters:
  1. `BASS VOL`: Walking Bass Level & Engine Name
  2. `KEYS VOL`: Comping Chord Level & Engine Name
  3. `DRUMS VOL`: Drum Kit Level & Kit Name
  4. `LEAD VOL`: Solo Lead Synth Level & Instrument Name
  5. `REVERB`: 4-Comb Schroeder Room Reverb Mix
  6. `SPICE`: Harmonic / Arrangement Complexity ($0\%\text{--}100\%$)
  7. `SWING`: Shuffle Swing Percentage ($50\%\text{--}80\%$)

---

## Sound Engines & Grooves

### 8 Selectable Groove Styles
Adjusted via **Encoder 0 (`SELECT`)**:
1. **12/8 Slow Blues**: Classic B.B. King / Buddy Guy slow triplet blues ballad with rolling 12/8 pulse.
2. **Texas Shuffle**: Driving Stevie Ray Vaughan / Albert Collins Texas shuffle with tight walking bass.
3. **Chicago Swing**: Muddy Waters / Little Walter bouncy, swinging shuffle with backbeat accents.
4. **Rockin' Blues**: Chuck Berry / Allman Brothers straight-8s driving blues-rock groove.
5. **New Orleans 2nd Line**: Professor Longhair syncopated street march beat with loose snare accents.
6. **Memphis Soul Blues**: Stax / Albert King 16th-note tight groove with locked bass stabs.
7. **Rhumba Blues**: B.B. King / Albert King *"Crosscut Saw"* 3-3-2 syncopated Afro-Cuban crosscut rhythm.
8. **Jump Blues / Boogie-Woogie**: Louis Jordan high-energy 8-to-the-bar walking bass and galloping swing drums.

---

### 4 Selectable Bass Engines
Adjusted via **Encoder 4 (`KNOB 3`)** with `GLO` held:
1. **Fender P-Bass**: Resonant multi-harmonic electric bass with ~50 ms initial pick bite snap, prominent 2nd & 3rd harmonics, and roundwound string decay. Audibly tuned to $E_2\text{--}D_4$ ($82\text{--}293\text{ Hz}$) for punchy clarity on the pocket speaker.
2. **Upright Double Bass**: Acoustic jazz/blues double bass with deep fundamental body resonance and percussive wooden fingerboard slap attack.
3. **Muted Tic-Tac Boogie**: Palm-muted baritone guitar / boogie bass with snappy pick attack and tight percussive decay (~120 ms).
4. **Moog Funk Synth**: Classic 24dB-style resonant analog synth bass with dynamic envelope filter sweep for 70s funk blues.

---

### 12 Selectable Chord Engines
Adjusted via **Encoder 1 (`ALGORITHM`)**:
1. **Rhodes FM (Algo 0)**: 2-Operator FM synthesis producing a sweet bell/tine attack that dissolves into a warm, mellow electric piano body.
2. **Bright FM EP (Algo 1)**: 2:1 FM chime algorithm with crisp attack transients and rich overtones.
3. **Wurlitzer 200A (Algo 2)**: Gritty reed electric piano with rich 3rd harmonic bite and warm asymmetric reed bark.
4. **Clean B3 Organ (Algo 3)**: Gospel jazz drawbars (88 8000 000 fundamental + octave + 5th) with smooth organ sustain.
5. **Dirty Gospel B3 (Algo 4)**: Saturated full blues drawbars (88 8800 000) with biting tube overdrive.
6. **Funk Clavinet D6 (Algo 5)**: Biting 3:1 plucked FM algorithm with fast percussive decay and sharp pick attack.
7. **Archtop Rhythm Guitar (Algo 6)**: Warm hollowbody jazz/blues comping guitar with string pluck transient and wooden acoustic resonance.
8. **Blues Horn Section (Algo 7)**: Tenor sax and trumpet brass ensemble with swelling attack and rich harmonized brass overtones.
9. **Honky-Tonk Piano (Algo 8)**: Barrelhouse upright acoustic piano with dual-chorus detuned strings and crisp hammer strike.
10. **Tremolo Suitcase EP (Algo 9)**: Warm electric piano with vintage optical tremolo amplitude modulation.
11. **Delta Resonator Guitar (Algo 10)**: Metallic acoustic Dobro body pluck with resonant acoustic mid-range bite.
12. **Rotary Leslie Organ (Algo 11)**: Gospel drawbar organ with dynamic dual-rotor rotary chorus swirl.

---

### 4 Selectable Drum Kits
Adjusted via **Encoder 0 (`SELECT`)** with `GLO` held:
1. **Studio Punch**: Deep acoustic kick punch (300 ms sub-sweep), crisp backbeat snare, sizzle shuffle hats, and woody rimshot.
2. **Vintage Brush**: Soft acoustic thud kick, textured sandpaper brush stir/swirl snare, and dark sizzle ride cymbal.
3. **Retro 808 / Lo-Fi**: Deep boomy 808 sub kick, snappy 808 clap/snare, sizzly metallic hats, and synthetic rim tick.
4. **Stax / Memphis Tight**: Dampened kick ("pillow in the kick drum"), tight dry snare crack, and crisp closed hats.

---

### 8 Solo Lead Voice Engines
Adjusted via direct turn of **Encoder 6 (`PRESETS`)**:
1. **Overdriven Blues Guitar**: Singing electric lead guitar with tube clipping bite, harmonic sustain, and touch sensitivity.
2. **Chicago Blues Harp**: Amplified green bullet harmonica with authentic nasal reed pulse, throat bite, and pitch-bend nuance.
3. **Tenor Saxophone**: Breathy acoustic tenor sax with acoustic throat resonance, expressive swell, and subtle reed growl.
4. **Moog Solo Lead**: Classic analog monosynth lead with detuned dual oscillators, singing filter resonance, and portamento glide.
5. **Warm Jazz Hollowbody ("Lucille")**: Mellow neck pickup tone with warm rolled-off tone pot and round singing fundamental.
6. **Fuzz Octave Stompbox**: Aggressive full-wave rectified octave-up distortion (Hendrix Band of Gypsys / Buddy Guy style).
7. **Muted Blues Trumpet**: Brassy harmon-muted trumpet with nasal odd harmonics and expressive formant bite.
8. **Percussive B3 Organ Lead**: Jimmy Smith style fast 3rd harmonic percussive key-click decaying into drawbar flutes.

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
- **Instant Save**: Press **`SAVE`** at any time to commit your current settings to the FM-1's non-volatile onboard flash storage. Background autosaving is intentionally disabled to protect your presets from accidental overwrites during live jamming.

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
