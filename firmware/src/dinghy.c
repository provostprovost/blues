/* SPDX-License-Identifier: GPL-3.0-only */
/* Blues Box Firmware for M-VAVE FM-1
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
                uint8_t open_hat = (frame.drum_mask & 0x04) && (g_harmony.variation > 75);
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

static void copy_preset_name(char *dst, const char *src) {
    int i = 0;
    while (src[i] && i < 19) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static void init_factory_presets(void) {
    static const char *FACTORY_NAMES[NUM_PRESET_SLOTS] = {
        "1: TEXAS SHUFFLE",
        "2: SLOW 12/8",
        "3: CHICAGO SWING",
        "4: ROCKIN 8S",
        "5: NOLA 2ND LINE",
        "6: MINOR BLUES",
        "7: MEMPHIS SOUL",
        "8: RHUMBA BLUES",
        "9: BOOGIE WOOGIE",
        "10: POCKET FUNK",
        "11: NOLA FUNK",
        "12: BLUES FUNK",
        "13: SLOW NEOFUNK",
        "14: COUNTRY TWNG",
        "15: PURDIE SHUF",
        "16: BO DIDDLEY"
    };
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        PresetConfig *p = &g_presets[i];
        p->magic = PRESET_MAGIC;
        p->preset_id = i;
        copy_preset_name(p->name, FACTORY_NAMES[i]);
        p->master_tuning = 0; // 440 Hz
        p->bass_vol = 96;
        p->keys_vol = 88;
        p->drums_vol = 96;
        p->lead_vol = 116;
        p->reverb_mix = 35;
        p->bass_cutoff = 85;
        p->drive_level = 20;
        p->count_in_enabled = 0;
        p->fullness = 65;
        p->opening_var = VAR_FLAT_FOUR;
        p->outro_type = OUTRO_TAG;
        p->bass_instrument = (uint8_t)(i % 4);
        p->drum_kit = (uint8_t)(i % 3);
        p->lead_instrument = (uint8_t)(i % 4);
        p->lead_play_style = LEAD_PLAY_POLY;
        p->easy_lead = 0;

        switch (i) {
            case 0: // 1: Texas Shuffle
                p->root_key = 4; // E
                p->groove_style = GROOVE_TEXAS_SHUFFLE;
                p->bpm = 112;
                p->variation = 65;
                p->swing = 67;
                p->ep_tone_algo = KEYS_RHODES;
                p->bass_instrument = BASS_P_BASS;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_BLUES_GUITAR;
                p->form_mode = FORM_MINOR_BLUES;
                break;
            case 1: // 2: Slow 12/8 Blues
                p->root_key = 4; // E
                p->groove_style = GROOVE_SLOW_12_8;
                p->bpm = 68;
                p->variation = 50;
                p->swing = 67;
                p->ep_tone_algo = KEYS_B3_DIRTY;
                p->bass_instrument = BASS_UPRIGHT;
                p->drum_kit = DRUM_BRUSH;
                p->lead_instrument = LEAD_HARMONICA;
                p->form_mode = FORM_12BAR;
                break;
            case 2: // 3: Chicago Swing
                p->root_key = 9; // A
                p->groove_style = GROOVE_CHICAGO_SWING;
                p->bpm = 130;
                p->variation = 55;
                p->swing = 67;
                p->ep_tone_algo = KEYS_BRIGHT_EP;
                p->bass_instrument = BASS_UPRIGHT;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_TENOR_SAX;
                p->form_mode = FORM_12BAR;
                break;
            case 3: // 4: Rockin' 8s
                p->root_key = 4; // E
                p->groove_style = GROOVE_STRAIGHT_ROCK;
                p->bpm = 140;
                p->variation = 40;
                p->swing = 0;
                p->ep_tone_algo = KEYS_CLAVINET;
                p->bass_instrument = BASS_MUTED_TICTAC;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_BLUES_GUITAR;
                p->form_mode = FORM_12BAR;
                break;
            case 4: // 5: New Orleans 2nd Line
                p->root_key = 7; // G
                p->groove_style = GROOVE_SECOND_LINE;
                p->bpm = 96;
                p->variation = 70;
                p->swing = 0;
                p->ep_tone_algo = KEYS_WURLI;
                p->bass_instrument = BASS_MOOG_FUNK;
                p->drum_kit = DRUM_RETRO_808;
                p->lead_instrument = LEAD_MOOG_LEAD;
                p->form_mode = FORM_12BAR;
                break;
            case 5: // 6: Minor Blues
                p->root_key = 0; // Cm
                p->groove_style = GROOVE_SLOW_12_8;
                p->bpm = 84;
                p->variation = 60;
                p->swing = 67;
                p->ep_tone_algo = KEYS_B3_CLEAN;
                p->bass_instrument = BASS_UPRIGHT;
                p->drum_kit = DRUM_BRUSH;
                p->lead_instrument = LEAD_HARMONICA;
                p->form_mode = FORM_MINOR_BLUES;
                break;
            case 6: // 7: Memphis Soul
                p->root_key = 5; // F
                p->groove_style = GROOVE_MEMPHIS_SOUL;
                p->bpm = 104;
                p->variation = 55;
                p->swing = 0;
                p->ep_tone_algo = KEYS_TREMOLO_EP;
                p->bass_instrument = BASS_P_BASS;
                p->drum_kit = DRUM_STAX_TIGHT;
                p->lead_instrument = LEAD_WARM_JAZZ;
                p->form_mode = FORM_12BAR;
                break;
            case 7: // 8: Rhumba Blues
                p->root_key = 2; // D
                p->groove_style = GROOVE_RHUMBA_BLUES;
                p->bpm = 118;
                p->variation = 50;
                p->swing = 0;
                p->ep_tone_algo = KEYS_RESONATOR;
                p->bass_instrument = BASS_P_BASS;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_MUTED_TRUMPET;
                p->form_mode = FORM_12BAR;
                break;
            case 8: // 9: Boogie Woogie
                p->root_key = 0; // C
                p->groove_style = GROOVE_BOOGIE_WOOGIE;
                p->bpm = 158;
                p->variation = 60;
                p->swing = 67;
                p->ep_tone_algo = KEYS_HONKY_TONK;
                p->bass_instrument = BASS_MUTED_TICTAC;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_PERC_ORGAN;
                p->form_mode = FORM_12BAR;
                break;
            case 9: // 10: 70s Pocket Funk
                p->root_key = 4; // E
                p->groove_style = GROOVE_POCKET_FUNK;
                p->bpm = 102;
                p->variation = 70;
                p->swing = 0;
                p->ep_tone_algo = KEYS_CLAVINET;
                p->bass_instrument = BASS_SLAP_POP;
                p->drum_kit = DRUM_STAX_TIGHT;
                p->lead_instrument = LEAD_TALKBOX;
                p->form_mode = FORM_12BAR;
                break;
            case 10: // 11: NOLA Meters Funk
                p->root_key = 7; // G
                p->groove_style = GROOVE_NOLA_FUNK;
                p->bpm = 94;
                p->variation = 70;
                p->swing = 0;
                p->ep_tone_algo = KEYS_WURLI;
                p->bass_instrument = BASS_MOOG_FUNK;
                p->drum_kit = DRUM_NEW_ORLEANS;
                p->lead_instrument = LEAD_HORN_SECT;
                p->form_mode = FORM_12BAR;
                break;
            case 11: // 12: Swung Blues-Funk
                p->root_key = 9; // A
                p->groove_style = GROOVE_BLUES_FUNK;
                p->bpm = 98;
                p->variation = 65;
                p->swing = 67;
                p->ep_tone_algo = KEYS_B3_DIRTY;
                p->bass_instrument = BASS_P_BASS;
                p->drum_kit = DRUM_DRY_VINTAGE;
                p->lead_instrument = LEAD_WAH_GUITAR;
                p->form_mode = FORM_12BAR;
                break;
            case 12: // 13: Slow Neo-Funk
                p->root_key = 5; // Fm
                p->groove_style = GROOVE_SLOW_NEO_FUNK;
                p->bpm = 78;
                p->variation = 65;
                p->swing = 0;
                p->ep_tone_algo = KEYS_RHODES;
                p->bass_instrument = BASS_SUB_OCTAVE;
                p->drum_kit = DRUM_LOFI_BOX;
                p->lead_instrument = LEAD_JAZZ_FLUTE;
                p->form_mode = FORM_MINOR_BLUES;
                break;
            case 13: // 14: Country Twang
                p->root_key = 7; // G
                p->groove_style = GROOVE_COUNTRY_TWANG;
                p->bpm = 128;
                p->variation = 55;
                p->swing = 67;
                p->ep_tone_algo = KEYS_HONKY_TONK;
                p->bass_instrument = BASS_MUTED_TICTAC;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_TELE_TWANG;
                p->form_mode = FORM_12BAR;
                break;
            case 14: // 15: Purdie Half-Time Shuffle
                p->root_key = 10; // Bb
                p->groove_style = GROOVE_PURDIE_SHUFFLE;
                p->bpm = 108;
                p->variation = 65;
                p->swing = 67;
                p->ep_tone_algo = KEYS_BRIGHT_EP;
                p->bass_instrument = BASS_JAZZ_60S;
                p->drum_kit = DRUM_STUDIO;
                p->lead_instrument = LEAD_PERC_ORGAN;
                p->form_mode = FORM_QUICK_CHANGE;
                break;
            case 15: // 16: Bo Diddley Clave
            default:
                p->root_key = 4; // E
                p->groove_style = GROOVE_BO_DIDDLEY;
                p->bpm = 124;
                p->variation = 60;
                p->swing = 0;
                p->ep_tone_algo = KEYS_TREMOLO_EP;
                p->bass_instrument = BASS_AMPEG_GROWL;
                p->drum_kit = DRUM_CHICAGO_ROOM;
                p->lead_instrument = LEAD_FUZZ_OCTAVE;
                p->form_mode = FORM_12BAR;
                break;
        }
        {
            uint8_t saved_swing = p->swing;
            HarmonyState tmp_hs;
            arranger_apply_groove_defaults(&tmp_hs, p, (GrooveStyle)p->groove_style);
            p->swing = saved_swing;
        }
        p->bass_fullness = p->fullness;
        p->keys_fullness = p->fullness;
        p->drums_fullness = p->fullness;
        p->bass_variation = p->variation;
        p->keys_variation = p->variation;
        p->drums_variation = p->variation;
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

    uint8_t saved_easy_lead = 0;
    GlobalSettings gs;
    if (hal_settings_load(&gs, sizeof(gs)) == 0 && gs.magic == SETTINGS_MAGIC) {
        g_active_preset = gs.active_preset % NUM_PRESET_SLOTS;
        saved_easy_lead = gs.easy_lead ? 1 : 0;
    }

    // 3. Initialize Harmony engine (defaults to STOPPED on boot)
    harmony_init(&g_harmony);
    g_harmony.is_playing = 0; // Stopped on boot: starts when BTN_PLAY or MIDI Start is received
    g_harmony.root_semitone = g_presets[g_active_preset].root_key;
    g_harmony.variation = g_presets[g_active_preset].variation;
    g_harmony.fullness = g_presets[g_active_preset].fullness;
    g_harmony.bass_fullness = g_presets[g_active_preset].bass_fullness;
    g_harmony.keys_fullness = g_presets[g_active_preset].keys_fullness;
    g_harmony.drums_fullness = g_presets[g_active_preset].drums_fullness;
    g_harmony.bass_variation = g_presets[g_active_preset].bass_variation;
    g_harmony.keys_variation = g_presets[g_active_preset].keys_variation;
    g_harmony.drums_variation = g_presets[g_active_preset].drums_variation;
    g_harmony.bass_style = g_presets[g_active_preset].bass_style;
    g_harmony.keys_style = g_presets[g_active_preset].keys_style;
    g_harmony.drum_pattern = g_presets[g_active_preset].drum_pattern;
    g_harmony.count_in_enabled = g_presets[g_active_preset].count_in_enabled;
    g_harmony.form_mode = (FormMode)g_presets[g_active_preset].form_mode;
    g_harmony.easy_lead = saved_easy_lead ? 1 : g_presets[g_active_preset].easy_lead;
    g_last_voicing.count = 0;

    // 4. Initialize Hardware Panels, UI & DSP
    panel_init();
    ui_init();
    dsp_init();
    dsp_set_lead_play_style(g_presets[g_active_preset].lead_play_style);

#ifndef DINGHY_HOST
    // 5. Short Skippable Boot Splash Screen (~900 ms, skipped on any key or button press)
    ui_render_splash();
    panel_update_leds(&g_harmony, g_active_preset);
    {
        uint32_t splash_start = hal_millis();
        hal_poll();
        (void)hal_get_btn_edges();
        uint32_t init_keys = hal_get_keys_mask();
        while (hal_millis() - splash_start < 900u) {
            hal_poll();
            if (hal_get_btn_edges() != 0 || (hal_get_keys_mask() & ~init_keys) != 0) {
                break;
            }
        }
    }
#endif

    // 6. Initial Screen Redraw
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

        // Flash Persistence: explicit commit via BTN_SAVE prevents accidental corruption
        // of factory presets during live jamming and temporary form variations.
        if (panel_check_autosave_needed(ms_now)) {
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
