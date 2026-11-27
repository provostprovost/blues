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
                /* a riff line says how long its note is (in 1/24 beats); the classic lines take the default gate */
                uint32_t gate = frame.bass_gate_ticks ? ((uint32_t)frame.bass_gate_ticks * 110250u) / bpm_valid : 0u;
                dsp_note_on_bass_ex(frame.bass_note, frame.bass_vel, preset->master_tuning, gate, frame.bass_ghost);
                midi_tx_push(0x91, frame.bass_note, frame.bass_vel); // Ch 2 Bass
                g_harmony.vis_bass_pulse = 3;
            }

            if (frame.comp_trigger) {
                dsp_note_on_keys(&frame.comp_voicing, frame.comp_vel, preset->master_tuning);
                g_last_voicing = frame.comp_voicing;
                for (uint8_t v = 0; v < frame.comp_voicing.count; v++) {
                    midi_tx_push(0x92, frame.comp_voicing.notes[v], frame.comp_vel); // Ch 3 Keys
                }
                g_harmony.vis_keys_pulse = 3;
            }
            if (g_harmony.count_in_bars_remaining > 0 &&
                g_harmony.active_intro_style == INTRO_DRONE_1BAR &&
                g_harmony.current_beat == 0 && g_harmony.current_subdiv == 0) {
                dsp_extend_drone_gate(110000u); // Hold Beat-1 drone across the full bar
            }

            if (frame.drum_mask) {
                /* General MIDI keys: kick, snare, closed hat, side stick, open hat, ride, crash, high and low tom */
                static const uint8_t GM_DRUM[NUM_DRUM_VOICES] = { 36, 38, 42, 37, 46, 51, 49, 48, 45 };
                dsp_trigger_drum(frame.drum_mask, frame.drum_vel);
                for (uint8_t d = 0; d < NUM_DRUM_VOICES; d++) {
                    if (frame.drum_mask & (1u << d)) midi_tx_push(0x99, GM_DRUM[d], frame.drum_vel[d]);
                }
                if (frame.drum_mask & ~((1u << DV_HAT) | (1u << DV_RIDE))) g_harmony.vis_drum_pulse = 3;
                else if (g_harmony.vis_drum_pulse == 0) g_harmony.vis_drum_pulse = 2;
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

/* The 16 factory presets (2.3: built on the real voices, the groove beats and the riff bass lines). A preset's bass
 * line, keys rhythm, drum beat and swing come from its groove (arranger_apply_groove_defaults). */
typedef struct {
    const char *name;
    uint8_t key, groove, bpm, variation, form;
    uint8_t keys, bass, kit, lead, play, easy;
    uint8_t fx1, fx1_amt, fx2, fx2_amt, slide;
} FactoryPreset;

static const FactoryPreset FACTORY[NUM_PRESET_SLOTS] = {
    /* name                key groove                 bpm var form               keys                 bass                kit            lead                 play                    easy fx1                amt fx2                amt slide */
    { "1: TEXAS SHUFFLE",  4, GROOVE_TEXAS_SHUFFLE,   118, 60, FORM_MINOR_BLUES,  KEYS_GRAND_STUDIO,   BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_TENOR,     LEAD_PLAY_LEGATO_GLIDE, 1,   LFX_SPRING_REVERB, 35, LFX_OFF,            0, 30 },
    { "2: SLOW 12/8",      9, GROOVE_SLOW_12_8,        62, 50, FORM_12BAR,        KEYS_TW_JIMMY,       BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR,    LEAD_PLAY_BLUES_BEND,   0,   LFX_OVERDRIVE,     45, LFX_SPRING_REVERB, 45, 34 },
    { "3: CHICAGO SHUFL",  9, GROOVE_CHICAGO_SWING,   126, 55, FORM_QUICK_CHANGE, KEYS_GRAND_JAZZ,     BASS_REAL_UPRIGHT,  DRUM_REAL_KIT, LEAD_HARMONICA,      LEAD_PLAY_BLUES_BEND,   0,   LFX_OVERDRIVE,     30, LFX_SLAPBACK,      40, 26 },
    { "4: ROCKIN 8S",      4, GROOVE_STRAIGHT_ROCK,   138, 45, FORM_12BAR,        KEYS_GRAND_STUDIO,   BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR_OD, LEAD_PLAY_POLY,         0,   LFX_DELAY,         30, LFX_OFF,            0, 20 },
    { "5: NOLA 2ND LINE",  7, GROOVE_SECOND_LINE,      96, 65, FORM_12BAR,        KEYS_GRAND_VINTAGE,  BASS_TUBA,          DRUM_REAL_KIT, LEAD_HORN_SECT,      LEAD_PLAY_POLY,         0,   LFX_SPRING_REVERB, 30, LFX_OFF,            0, 20 },
    { "6: MINOR BLUES",    0, GROOVE_SLOW_12_8,        76, 55, FORM_MINOR_BLUES,  KEYS_TW_BALLAD,      BASS_REAL_UPRIGHT,  DRUM_REAL_KIT, LEAD_REAL_TENOR,     LEAD_PLAY_LEGATO_GLIDE, 0,   LFX_SPRING_REVERB, 50, LFX_OFF,            0, 40 },
    { "7: MEMPHIS SOUL",   5, GROOVE_MEMPHIS_SOUL,    104, 50, FORM_12BAR,        KEYS_TW_JIMMY,       BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR,    LEAD_PLAY_POLY,         0,   LFX_TREMOLO,       45, LFX_SPRING_REVERB, 35, 20 },
    { "8: RHUMBA BLUES",   2, GROOVE_RHUMBA_BLUES,    118, 50, FORM_12BAR,        KEYS_GRAND_VINTAGE,  BASS_REAL_UPRIGHT,  DRUM_REAL_KIT, LEAD_MUTED_TRUMPET,  LEAD_PLAY_BLUES_BEND,   0,   LFX_SPRING_REVERB, 40, LFX_OFF,            0, 26 },
    { "9: BOOGIE WOOGIE",  0, GROOVE_BOOGIE_WOOGIE,   156, 55, FORM_12BAR,        KEYS_GRAND_STUDIO,   BASS_REAL_UPRIGHT,  DRUM_REAL_KIT, LEAD_REAL_TENOR,     LEAD_PLAY_POLY,         0,   LFX_SLAPBACK,      35, LFX_OFF,            0, 16 },
    { "10: POCKET FUNK",   4, GROOVE_POCKET_FUNK,     100, 65, FORM_12BAR,        KEYS_CLAVINET,       BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR,    LEAD_PLAY_POLY,         0,   LFX_COMPRESSOR,    50, LFX_AUTOWAH,       70, 16 },
    { "11: NOLA FUNK",     7, GROOVE_NOLA_FUNK,        94, 65, FORM_12BAR,        KEYS_WURLI,          BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_HORN_SECT,      LEAD_PLAY_POLY,         0,   LFX_SLAPBACK,      30, LFX_OFF,            0, 20 },
    { "12: BLUES FUNK",    9, GROOVE_BLUES_FUNK,       98, 60, FORM_12BAR,        KEYS_TW_GOSPEL,      BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR_OD, LEAD_PLAY_BLUES_BEND,   0,   LFX_UNIVIBE,       45, LFX_SPRING_REVERB, 30, 26 },
    { "13: SLOW NEOFUNK",  5, GROOVE_SLOW_NEO_FUNK,    78, 60, FORM_MINOR_BLUES,  KEYS_RHODES,         BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_TENOR,     LEAD_PLAY_LEGATO_GLIDE, 0,   LFX_CHORUS,        45, LFX_DELAY,         35, 45 },
    { "14: COUNTRY TWNG",  7, GROOVE_COUNTRY_TWANG,   128, 50, FORM_12BAR,        KEYS_GRAND_VINTAGE,  BASS_REAL_UPRIGHT,  DRUM_REAL_KIT, LEAD_TELE_TWANG,     LEAD_PLAY_BLUES_BEND,   0,   LFX_COMPRESSOR,    40, LFX_SLAPBACK,      45, 22 },
    { "15: PURDIE SHUF",  10, GROOVE_PURDIE_SHUFFLE,   96, 65, FORM_QUICK_CHANGE, KEYS_TREMOLO_EP,     BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR,    LEAD_PLAY_POLY,         0,   LFX_PHASER,        40, LFX_SPRING_REVERB, 30, 20 },
    { "16: BO DIDDLEY",    4, GROOVE_BO_DIDDLEY,      122, 55, FORM_12BAR,        KEYS_GRAND_STUDIO,   BASS_REAL_ELECTRIC, DRUM_REAL_KIT, LEAD_REAL_GUITAR_OD, LEAD_PLAY_POLY,         0,   LFX_TREMOLO,       70, LFX_OFF,            0, 16 }
};

static void init_factory_presets(void) {
    for (uint8_t i = 0; i < NUM_PRESET_SLOTS; i++) {
        const FactoryPreset *f = &FACTORY[i];
        PresetConfig *p = &g_presets[i];
        p->magic = PRESET_MAGIC;
        p->preset_id = i;
        copy_preset_name(p->name, f->name);
        p->master_tuning = 0; // 440 Hz
        p->bass_vol = 96;
        p->keys_vol = 88;
        p->drums_vol = 96;
        p->lead_vol = 116;
        p->reverb_mix = 35;
        p->bass_cutoff = 85;
        p->drive_level = 20;
        p->count_in_enabled = INTRO_AUTO;
        p->fullness = 65;
        p->opening_var = VAR_FLAT_FOUR;
        p->outro_type = OUTRO_CLASSIC_SLIDE;
        p->heat_level = 3;
        p->root_key = f->key;
        p->groove_style = f->groove;
        p->bpm = f->bpm;
        p->variation = f->variation;
        p->form_mode = f->form;
        p->ep_tone_algo = f->keys;
        p->bass_instrument = f->bass;
        p->drum_kit = f->kit;
        p->lead_instrument = f->lead;
        p->lead_play_style = f->play;
        p->easy_lead = f->easy;
        p->lead_fx1 = f->fx1;
        p->lead_fx1_amt = f->fx1_amt;
        p->lead_fx2 = f->fx2;
        p->lead_fx2_amt = f->fx2_amt;
        p->lead_slide = f->slide;
        {
            HarmonyState tmp_hs;
            arranger_apply_groove_defaults(&tmp_hs, p, (GrooveStyle)p->groove_style);
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

    uint8_t saved_easy_lead = 0, saved_lead_home = 0;
    int8_t saved_lead_transpose = 0;
    GlobalSettings gs;
    int gs_ok = (hal_settings_load(&gs, sizeof(gs)) == 0);
    if (!gs_ok && hal_settings_load(&gs, GLOBAL_SETTINGS_V1_SIZE) == 0) {   /* saved by 2.3 or before */
        gs.lead_home = 0;
        gs.lead_transpose = 0;
        gs_ok = 1;
    }
    if (gs_ok && gs.magic == SETTINGS_MAGIC) {
        g_active_preset = gs.active_preset % NUM_PRESET_SLOTS;
        saved_easy_lead = gs.easy_lead ? 1 : 0;
        saved_lead_home = (gs.lead_home <= 12) ? gs.lead_home : 0;
        saved_lead_transpose = (gs.lead_transpose >= -12 && gs.lead_transpose <= 12) ? gs.lead_transpose : 0;
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
    g_harmony.lead_home = saved_lead_home;
    g_harmony.lead_transpose = saved_lead_transpose;
    uint8_t hl = g_presets[g_active_preset].heat_level;
    g_harmony.heat_level = (hl >= 1 && hl <= 5) ? hl : 3;
    g_last_voicing.count = 0;

    // 4. Initialize Hardware Panels, UI & DSP
    panel_init();
    ui_init();
    dsp_init();
    dsp_set_lead_play_style(g_presets[g_active_preset].lead_play_style);

#ifndef DINGHY_HOST
    // 5. Skippable Boot Splash Screen (~2000 ms, skipped on any key or button press)
    ui_render_splash();
    panel_update_leds(&g_harmony, g_active_preset);
    {
        uint32_t splash_start = hal_millis();
        hal_poll();
        (void)hal_get_btn_edges();
        uint32_t init_keys = hal_get_keys_mask();
        while (hal_millis() - splash_start < 2000u) {
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
        g_presets[g_active_preset].heat_level = g_harmony.heat_level;

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
            if (g_harmony.vis_bass_pulse > 0) g_harmony.vis_bass_pulse--;
            if (g_harmony.vis_keys_pulse > 0) g_harmony.vis_keys_pulse--;
            if (g_harmony.vis_drum_pulse > 0) g_harmony.vis_drum_pulse--;
            g_harmony.ui_dirty = 0;
        }
    }
}
