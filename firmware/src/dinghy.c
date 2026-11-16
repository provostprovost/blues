/* SPDX-License-Identifier: GPL-3.0-only */
/* TexasBlues-1 (TB-1) Firmware for M-VAVE FM-1
 * Phase 5: Full-System Integration, MIDI Ring Buffer & Cooperative Engine
 */

static void app_idle(void);
#define LCD_IDLE app_idle
#include "platform.c"

#include "blues_types.h"
#include "blues_harmony.h"
#include "blues_arranger.h"
#include "dsp_voices.h"
#include "panel_input.h"
#include "ui_screens.h"
#include "hal_adapter.h"

#include "blues_harmony.c"
#include "blues_arranger.c"
#include "dsp_voices.c"
#include "panel_input.c"
#include "ui_screens.c"

// Lock-Free MIDI Out Ring Buffer
#define MIDI_RING_SIZE 64
typedef struct {
    uint8_t status;
    uint8_t d1;
    uint8_t d2;
} MidiEvent;

static MidiEvent        g_midi_tx_ring[MIDI_RING_SIZE];
static volatile uint8_t g_midi_tx_head = 0;
static volatile uint8_t g_midi_tx_tail = 0;

static inline void midi_tx_push(uint8_t status, uint8_t d1, uint8_t d2) {
    uint8_t next = (uint8_t)((g_midi_tx_head + 1) % MIDI_RING_SIZE);
    if (next != g_midi_tx_tail) {
        g_midi_tx_ring[g_midi_tx_head].status = status;
        g_midi_tx_ring[g_midi_tx_head].d1 = d1;
        g_midi_tx_ring[g_midi_tx_head].d2 = d2;
        g_midi_tx_head = next;
    }
}

static inline void midi_tx_flush(void) {
    while (g_midi_tx_tail != g_midi_tx_head) {
        MidiEvent ev = g_midi_tx_ring[g_midi_tx_tail];
        g_midi_tx_tail = (uint8_t)((g_midi_tx_tail + 1) % MIDI_RING_SIZE);
        hal_midi_send(ev.status, ev.d1, ev.d2);
    }
}

static HarmonyState  g_harmony;
static PresetConfig  g_presets[NUM_PRESET_SLOTS];
static uint8_t       g_active_preset = 0;
static uint8_t       g_ui_screen = 0;
static Voicing       g_last_voicing;
static uint32_t      g_sample_acc = 2646000;
static uint32_t      g_last_redraw = 0;

static void app_idle(void) {
    /* Called during screen DMA refresh - process high-priority MIDI */
    midi_tx_flush();
    uint8_t st, d1, d2;
    while (hal_midi_get(&st, &d1, &d2)) {
        panel_handle_midi_in(&g_harmony, st, d1, d2);
    }
}

/* Audio ISR (256 stereo frames per block, ~5.8 ms budget) */
static void app_audio(int32_t *out, uint32_t n) {
    const PresetConfig *preset = &g_presets[g_active_preset];

    if (!g_harmony.is_playing) {
        dsp_all_notes_off();
        dsp_render_block(out, n, preset);
        return;
    }

    uint32_t bpm_valid = (preset->bpm < 40) ? 112 : preset->bpm;
    for (uint32_t f = 0; f < n; f++) {
        g_sample_acc += bpm_valid * 24;
        if (g_sample_acc >= 2646000) {
            g_sample_acc -= 2646000;

            PerformanceFrame frame;
            arranger_generate_frame(&g_harmony, preset, &frame, &g_last_voicing);

            if (frame.bass_trigger) {
                dsp_note_on_bass(frame.bass_note, frame.bass_vel, preset->master_tuning);
                midi_tx_push(0x91, frame.bass_note, frame.bass_vel); // Ch 2 Bass
            }

            if (frame.comp_trigger) {
                dsp_note_on_keys(&frame.comp_voicing, frame.comp_vel, preset->master_tuning);
                g_last_voicing = frame.comp_voicing;
                for (uint8_t v = 0; v < frame.comp_voicing.count; v++) {
                    midi_tx_push(0x92, frame.comp_voicing.notes[v], frame.comp_vel); // Ch 3 Keys
                }
            }

            if (frame.drum_mask) {
                uint8_t open_hat = (frame.drum_mask & 0x04) && (g_harmony.spice_level > 75);
                dsp_trigger_drum(frame.drum_mask, frame.drum_vel, open_hat);

                if (frame.drum_mask & 0x01) midi_tx_push(0x99, 36, frame.drum_vel[0]); // Kick
                if (frame.drum_mask & 0x02) midi_tx_push(0x99, 38, frame.drum_vel[1]); // Snare
                if (frame.drum_mask & 0x04) midi_tx_push(0x99, open_hat ? 46 : 42, frame.drum_vel[2]); // Hat
                if (frame.drum_mask & 0x08) midi_tx_push(0x99, 37, frame.drum_vel[3]); // Rimshot
            }

            // MIDI Clock 0xF8
            midi_tx_push(0xF8, 0, 0);

            harmony_tick_subdivision(&g_harmony);
        }
    }

    dsp_render_block(out, n, preset);
}

static void init_factory_presets(void) {
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        PresetConfig *p = &g_presets[i];
        p->magic = PRESET_MAGIC;
        p->preset_id = i;
        p->master_tuning = 0; // 440 Hz
        p->bass_vol = 110;
        p->keys_vol = 95;
        p->drums_vol = 105;
        p->reverb_mix = 35;
        p->bass_cutoff = 85;
        p->drive_level = 20;
        p->count_in_enabled = 0;
        p->opening_var = VAR_FLAT_FOUR;
        p->outro_type = OUTRO_TAG;

        switch (i) {
            case 0: // Texas Shuffle
                p->root_key = 4; // E
                p->groove_style = GROOVE_TEXAS_SHUFFLE;
                p->bpm = 112;
                p->spice = 65;
                p->swing = 67;
                p->ep_tone_algo = 0; // Rhodes FM
                p->form_mode = FORM_12BAR;
                break;
            case 1: // Slow 12/8 Blues
                p->root_key = 4; // E
                p->groove_style = GROOVE_SLOW_12_8;
                p->bpm = 68;
                p->spice = 50;
                p->swing = 67;
                p->ep_tone_algo = 2; // B3 Organ
                p->form_mode = FORM_12BAR;
                break;
            case 2: // Chicago Swing
                p->root_key = 4; // E
                p->groove_style = GROOVE_CHICAGO_SWING;
                p->bpm = 130;
                p->spice = 55;
                p->swing = 60;
                p->ep_tone_algo = 1; // Bright EP
                p->form_mode = FORM_12BAR;
                break;
            case 3: // Rockin' 8s
                p->root_key = 4; // E
                p->groove_style = GROOVE_STRAIGHT_ROCK;
                p->bpm = 140;
                p->spice = 40;
                p->swing = 50;
                p->ep_tone_algo = 3; // Clavinet
                p->form_mode = FORM_12BAR;
                break;
            case 4: // New Orleans 2nd Line
                p->root_key = 4; // E
                p->groove_style = GROOVE_SECOND_LINE;
                p->bpm = 96;
                p->spice = 70;
                p->swing = 55;
                p->ep_tone_algo = 0; // Rhodes FM
                p->form_mode = FORM_12BAR;
                break;
            case 5: // Minor Blues
                p->root_key = 4; // Em
                p->groove_style = GROOVE_SLOW_12_8;
                p->bpm = 84;
                p->spice = 60;
                p->swing = 67;
                p->ep_tone_algo = 2; // B3 Organ
                p->form_mode = FORM_MINOR_BLUES;
                break;
            default: // Varied Blues Jam slots
                p->root_key = 4; // E
                p->groove_style = (uint8_t)(i % 5);
                p->bpm = (uint8_t)(90 + (i * 6));
                p->spice = 50;
                p->swing = 65;
                p->ep_tone_algo = (uint8_t)(i % 4);
                p->form_mode = (i % 3 == 0) ? FORM_MINOR_BLUES : FORM_12BAR;
                break;
        }
    }
}

static void app_main(void) {
    // 1. Initialize factory presets
    init_factory_presets();

    // 2. Load stored presets from flash Blob 0 (if valid)
    PresetConfig flash_presets[NUM_PRESET_SLOTS];
    if (hal_blob_load(0, flash_presets, sizeof(flash_presets)) == 0) {
        if (flash_presets[0].magic == PRESET_MAGIC) {
            for (int i = 0; i < NUM_PRESET_SLOTS; i++) {
                g_presets[i] = flash_presets[i];
            }
        }
    }

    // 3. Initialize Harmony engine (defaults to STOPPED on boot)
    harmony_init(&g_harmony);
    g_harmony.is_playing = 0; // Stopped on boot: starts when BTN_PLAY or MIDI Start is received
    g_harmony.root_semitone = g_presets[g_active_preset].root_key;
    g_harmony.spice_level = g_presets[g_active_preset].spice;
    g_harmony.form_mode = (FormMode)g_presets[g_active_preset].form_mode;
    g_last_voicing.count = 0;

    // 4. Initialize Hardware Panels, UI & DSP
    panel_init();
    ui_init();
    dsp_init();

    // 5. Initial Screen Redraw
    ui_render(&g_harmony, &g_presets[g_active_preset], g_active_preset, g_ui_screen, 1);

    // 6. Main Cooperative Superloop
    for (;;) {
        hal_poll();
        uint32_t ms_now = hal_millis();

        // Drain MIDI IN messages
        uint8_t mst, md1, md2;
        while (hal_midi_get(&mst, &md1, &md2)) {
            panel_handle_midi_in(&g_harmony, mst, md1, md2);
        }

        // Poll Panel Inputs (Buttons, Encoders, Silicone Keys)
        panel_poll_inputs(&g_harmony, g_presets, &g_active_preset, ms_now, &g_ui_screen);

        // Flush queued MIDI events to USB-MIDI
        midi_tx_flush();

        // Update Button & Status LEDs
        panel_update_leds(&g_harmony, g_active_preset);

        // Debounced Autosave (strictly executed when playback is silent/stopped)
        if (panel_check_autosave_needed(ms_now) && !g_harmony.is_playing) {
            hal_blob_save(0, g_presets, sizeof(g_presets));
            panel_clear_autosave_needed();
        }

        // Screen Redraw Throttle (~30 Hz)
        if (ms_now - g_last_redraw >= 33) {
            g_last_redraw = ms_now;
            ui_render(&g_harmony, &g_presets[g_active_preset], g_active_preset, g_ui_screen, 0);
            g_harmony.ui_dirty = 0;
        }
    }
}
