#include "panel_input.h"
#include "hal_adapter.h"
#include "dsp_voices.h"
#include "blues_harmony.h"

#define ZONE1_COUNT 12u
#define KEY0_OFFSET 53u // F3 (MIDI 53)
#define KEY_LEAD_BASE 65u // F4 (MIDI 65) - 2-octave playable solo keyboard (F4..G6)

static uint32_t g_prev_key_mask = 0;
static uint32_t g_last_key_press_ms[ZONE1_COUNT] = {0};
static uint8_t  g_save_pending = 0;
static uint32_t g_last_param_change_ms = 0;
static uint32_t g_last_tap_ms = 0;
static int8_t   g_lead_octave_shift = 0;

static void set_toast(HarmonyState *hs, uint32_t ms_now, const char *msg) {
    int i = 0;
    while (msg[i] && i < 27) {
        hs->toast_msg[i] = msg[i];
        i++;
    }
    hs->toast_msg[i] = '\0';
    hs->toast_until_ms = ms_now + 1500u;
    hs->ui_dirty = 1;
}

static void set_toast_val(HarmonyState *hs, uint32_t ms_now, const char *prefix, int32_t val, const char *suffix) {
    char buf[28];
    int i = 0;
    while (prefix[i] && i < 18) { buf[i] = prefix[i]; i++; }
    if (val < 0) { buf[i++] = '-'; val = -val; }
    if (val >= 100) buf[i++] = (char)('0' + (val / 100) % 10);
    if (val >= 10)  buf[i++] = (char)('0' + (val / 10) % 10);
    buf[i++] = (char)('0' + (val % 10));
    int j = 0;
    while (suffix[j] && i < 27) { buf[i++] = suffix[j++]; }
    buf[i] = '\0';
    set_toast(hs, ms_now, buf);
}

static uint8_t clamp_step_u8(uint8_t cur, int32_t delta_step, uint8_t min_v, uint8_t max_v) {
    int32_t nv = (int32_t)cur + delta_step;
    if (nv < (int32_t)min_v) nv = min_v;
    if (nv > (int32_t)max_v) nv = max_v;
    return (uint8_t)nv;
}

void panel_init(void) {
    g_prev_key_mask = 0;
    for (uint32_t i = 0; i < ZONE1_COUNT; i++) {
        g_last_key_press_ms[i] = 0;
    }
    g_save_pending = 0;
    g_last_param_change_ms = 0;
    g_last_tap_ms = 0;
    g_lead_octave_shift = 0;
}

uint8_t panel_check_autosave_needed(uint32_t ms_now) {
    if (g_save_pending && (ms_now - g_last_param_change_ms >= AUTOSAVE_DEBOUNCE_MS)) {
        return 1;
    }
    return 0;
}

void panel_clear_autosave_needed(void) {
    g_save_pending = 0;
}

void panel_handle_midi_in(HarmonyState *hs, uint8_t status, uint8_t d1, uint8_t d2) {
    if (status == 0xFA) { // Real-Time Start
        hs->is_playing = 1;
        hs->current_bar = 0;
        hs->current_beat = 0;
        hs->current_subdiv = 0;
        hs->ui_dirty = 1;
        return;
    }
    if (status == 0xFC) { // Real-Time Stop
        hs->is_playing = 0;
        dsp_all_notes_off();
        hs->ui_dirty = 1;
        return;
    }
    if (status == 0xF8) { // Real-Time Clock
        hs->ext_clock_active = 1;
        hs->ext_clock_ticks_pending++;
        hs->ext_clock_timeout_ms = 500;
        return;
    }

    uint8_t msg_type = status & 0xF0;
    if (msg_type == 0xB0) { // Control Change
        switch (d1) {
            case 64: // Sustain Pedal -> Hold Chord
                hs->hold_chord_active = (d2 >= 64) ? 1 : 0;
                hs->ui_dirty = 1;
                break;
            case 80: // Footswitch 1 -> Early Turnaround
                if (d2 >= 64) {
                    hs->queued_cue = CUE_FORCE_TURNAROUND;
                    hs->ui_dirty = 1;
                }
                break;
            case 81: // Footswitch 2 -> Stop Time Break
                if (d2 >= 64) {
                    hs->queued_cue = CUE_STOP_TIME;
                    hs->ui_dirty = 1;
                }
                break;
            case 82: // Footswitch 3 -> Vamp Toggle
                if (d2 >= 64) {
                    hs->queued_cue = (hs->is_vamp_mode ? CUE_TO_12BAR : CUE_TO_VAMP);
                    hs->ui_dirty = 1;
                }
                break;
            case 83: // Footswitch 4 -> End Song
                if (d2 >= 64) {
                    hs->ending_armed = 1;
                    hs->outro_type = OUTRO_STOP;
                    hs->ui_dirty = 1;
                }
                break;
            default:
                break;
        }
    }
}

void panel_poll_inputs(HarmonyState *hs, PresetConfig *presets, uint8_t *active_preset_idx, uint32_t ms_now, uint8_t *ui_screen) {
    PresetConfig *p = &presets[*active_preset_idx];
    uint8_t glo_held = hal_get_glo_held();
    uint8_t sel_held = (hal_get_btns_mask() & (1u << BTN_SEL)) != 0;

    // 1. Buttons
    uint16_t btn_edges = hal_get_btn_edges();
    if (btn_edges & (1u << BTN_PLAY)) {
        hs->is_playing = !hs->is_playing;
        if (hs->is_playing) {
            hs->current_bar = 0;
            hs->current_beat = 0;
            hs->current_subdiv = 0;
        } else {
            dsp_all_notes_off();
        }
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_REC)) {
        hs->count_in_enabled = !hs->count_in_enabled;
        p->count_in_enabled = hs->count_in_enabled;
        set_toast(hs, ms_now, hs->count_in_enabled ? "COUNT-IN: ON" : "COUNT-IN: OFF");
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }
    if (btn_edges & (1u << BTN_HOME)) {
        if (ui_screen) {
            *ui_screen = !(*ui_screen);
            hs->ui_dirty = 1;
        }
    }
    if (btn_edges & (1u << BTN_OCT_DOWN)) {
        if (sel_held) {
            p->bpm = clamp_step_u8(p->bpm, -4, 40, 240);
            set_toast_val(hs, ms_now, "BPM: ", p->bpm, "");
        } else {
            if (g_lead_octave_shift > -2) g_lead_octave_shift--;
            set_toast_val(hs, ms_now, "OCTAVE: ", g_lead_octave_shift, "");
        }
    }
    if (btn_edges & (1u << BTN_OCT_UP)) {
        if (sel_held) {
            p->bpm = clamp_step_u8(p->bpm, 4, 40, 240);
            set_toast_val(hs, ms_now, "BPM: ", p->bpm, "");
        } else {
            if (g_lead_octave_shift < 2) g_lead_octave_shift++;
            set_toast_val(hs, ms_now, (g_lead_octave_shift > 0) ? "OCTAVE: +" : "OCTAVE: ", g_lead_octave_shift, "");
        }
    }
    if (btn_edges & (1u << BTN_SAVE)) {
        // Immediate explicit preset save
        p->root_key = hs->root_semitone;
        p->spice = hs->spice_level;
        p->fullness = hs->fullness;
        p->form_mode = hs->form_mode;
        hal_blob_save(0, presets, sizeof(PresetConfig) * NUM_PRESET_SLOTS);
        g_save_pending = 0;
        set_toast(hs, ms_now, "SAVED TO FLASH!");
    }
    if (btn_edges & (1u << BTN_ARP)) {
        // Tap Tempo on ARP button
        if (g_last_tap_ms > 0 && (ms_now - g_last_tap_ms >= 200) && (ms_now - g_last_tap_ms <= 2000)) {
            uint32_t interval = ms_now - g_last_tap_ms;
            uint32_t tap_bpm = 60000 / interval;
            if (tap_bpm >= 40 && tap_bpm <= 240) {
                p->bpm = (uint8_t)tap_bpm;
                hs->ui_dirty = 1;
                g_save_pending = 1;
                g_last_param_change_ms = ms_now;
            }
        }
        g_last_tap_ms = ms_now;
    }
    if (btn_edges & (1u << BTN_FX)) {
        // Instant Manual Accent Stab
        hs->manual_stab_pending = 1;
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_EDIT)) {
        // Early Turnaround Cue
        hs->queued_cue = CUE_FORCE_TURNAROUND;
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_ENV)) {
        // Toggle Vamp Mode on current chord
        hs->queued_cue = (hs->is_vamp_mode ? CUE_TO_12BAR : CUE_TO_VAMP);
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_SEQ)) {
        // Trigger 4-Bar Stop-Time Break
        hs->queued_cue = CUE_STOP_TIME;
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_LFO)) {
        // Toggle Minor Blues Form
        if (hs->form_mode == FORM_MINOR_BLUES) {
            hs->form_mode = FORM_12BAR;
        } else {
            hs->form_mode = FORM_MINOR_BLUES;
        }
        p->form_mode = hs->form_mode;
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // 2. Rotary Encoders
    int8_t delta;

    // ENC_PRESETS (Encoder 6): Select Lead Instrument (0..7), or Root Key with SEL or GLO held
    if ((delta = hal_get_encoder_delta(ENC_PRESETS)) != 0) {
        if (sel_held || glo_held) {
            int32_t nk = (int32_t)hs->root_semitone + delta;
            while (nk < 0) nk += 12;
            hs->root_semitone = (uint8_t)(nk % 12);
            p->root_key = hs->root_semitone;
            hs->ui_dirty = 1;
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else {
            int32_t nl = (int32_t)p->lead_instrument + delta;
            while (nl < 0) nl += 8;
            p->lead_instrument = (uint8_t)(nl % 8);
            hs->ui_dirty = 1;
        }
    }

    // ENC_ALGORITHM (Encoder 1): Keys Sound Algorithm (0..11)
    if ((delta = hal_get_encoder_delta(ENC_ALGORITHM)) != 0) {
        int32_t na = (int32_t)p->ep_tone_algo + delta;
        while (na < 0) na += 12;
        p->ep_tone_algo = (uint8_t)(na % 12);
        hs->ui_dirty = 1;
    }

    // ENC_SELECT (Encoder 0): Groove Style (0..7), Drum Kit with GLO (0..3), or Form/Scale Mode with SEL (0..4)
    if ((delta = hal_get_encoder_delta(ENC_SELECT)) != 0) {
        if (glo_held) {
            int32_t nd = (int32_t)p->drum_kit + delta;
            while (nd < 0) nd += 4;
            p->drum_kit = (uint8_t)(nd % 4);
        } else if (sel_held) {
            int32_t nf = (int32_t)hs->form_mode + delta;
            while (nf < 0) nf += 5;
            hs->form_mode = (FormMode)(nf % 5);
            p->form_mode = hs->form_mode;
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else {
            int32_t ng = (int32_t)p->groove_style + delta;
            while (ng < 0) ng += 8;
            p->groove_style = (uint8_t)(ng % 8);
        }
        hs->ui_dirty = 1;
    }

    uint8_t is_mixer = (ui_screen && *ui_screen == 1);

    static const char *TUNE_LABELS[3] = { "TUNE: 440 HZ", "TUNE: 432 HZ", "TUNE: 442 HZ" };
    static const char *BASS_LABELS[4] = { "BASS: P-BASS", "BASS: UPRIGHT", "BASS: MUTED BOOGIE", "BASS: MOOG FUNK" };

    // ENC_KNOB1 (Encoder 2): BPM / Swing (or Bass Vol / Reverb Mix on Mixer view)
    if ((delta = hal_get_encoder_delta(ENC_KNOB1)) != 0) {
        if (is_mixer) {
            if (glo_held) p->reverb_mix = clamp_step_u8(p->reverb_mix, delta * 4, 0, 127);
            else          p->bass_vol   = clamp_step_u8(p->bass_vol,   delta * 4, 0, 127);
        } else {
            if (glo_held) {
                p->swing = clamp_step_u8(p->swing, delta * 2, 0, 100);
                set_toast_val(hs, ms_now, "SWING: ", p->swing, "%");
            } else {
                p->bpm = clamp_step_u8(p->bpm, delta * 2, 40, 240);
            }
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // ENC_KNOB2 (Encoder 3): Spice / Tuning / Fullness (SEL) (or Keys Vol / Spice on Mixer view)
    if ((delta = hal_get_encoder_delta(ENC_KNOB2)) != 0) {
        if (sel_held) {
            hs->fullness = clamp_step_u8(hs->fullness, delta * 5, 0, 100);
            p->fullness = hs->fullness;
            set_toast_val(hs, ms_now, "FULLNESS: ", hs->fullness, "%");
        } else if (is_mixer) {
            if (glo_held) {
                hs->spice_level = clamp_step_u8(hs->spice_level, delta * 5, 0, 100);
                p->spice = hs->spice_level;
            } else {
                p->keys_vol = clamp_step_u8(p->keys_vol, delta * 4, 0, 127);
            }
        } else {
            if (glo_held) {
                int32_t nt = (int32_t)p->master_tuning + delta;
                while (nt < 0) nt += 3;
                p->master_tuning = (uint8_t)(nt % 3);
                set_toast(hs, ms_now, TUNE_LABELS[p->master_tuning]);
            } else {
                hs->spice_level = clamp_step_u8(hs->spice_level, delta * 5, 0, 100);
                p->spice = hs->spice_level;
            }
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // ENC_KNOB3 (Encoder 4): Bass Vol / Bass Engine (or Drums Vol / Swing on Mixer view)
    if ((delta = hal_get_encoder_delta(ENC_KNOB3)) != 0) {
        if (is_mixer) {
            if (glo_held) p->swing     = clamp_step_u8(p->swing,     delta * 2, 0, 100);
            else          p->drums_vol = clamp_step_u8(p->drums_vol, delta * 4, 0, 127);
        } else {
            if (glo_held) {
                int32_t nb = (int32_t)p->bass_instrument + delta;
                while (nb < 0) nb += 4;
                p->bass_instrument = (uint8_t)(nb % 4);
                set_toast(hs, ms_now, BASS_LABELS[p->bass_instrument]);
            } else {
                p->bass_vol = clamp_step_u8(p->bass_vol, delta * 4, 0, 127);
                set_toast_val(hs, ms_now, "BASS VOL: ", (int32_t)p->bass_vol * 100 / 127, "%");
            }
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // ENC_KNOB4 (Encoder 5): Keys Vol / Reverb Mix (or Lead Vol / Tuning on Mixer view)
    if ((delta = hal_get_encoder_delta(ENC_KNOB4)) != 0) {
        if (is_mixer) {
            if (glo_held) {
                int32_t nt = (int32_t)p->master_tuning + delta;
                while (nt < 0) nt += 3;
                p->master_tuning = (uint8_t)(nt % 3);
            } else {
                p->lead_vol = clamp_step_u8(p->lead_vol, delta * 4, 0, 127);
            }
        } else {
            if (glo_held) {
                p->reverb_mix = clamp_step_u8(p->reverb_mix, delta * 4, 0, 127);
                set_toast_val(hs, ms_now, "REVERB: ", (int32_t)p->reverb_mix * 100 / 127, "%");
            } else {
                p->keys_vol = clamp_step_u8(p->keys_vol, delta * 4, 0, 127);
                set_toast_val(hs, ms_now, "KEYS VOL: ", (int32_t)p->keys_vol * 100 / 127, "%");
            }
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // 3. Silicone Keys: 27 Keys (Playable Lead Keyboard unless SEL is held)
    uint32_t curr_key_mask = hal_get_keys_mask();
    uint32_t pressed = curr_key_mask & ~g_prev_key_mask;
    uint32_t released = ~curr_key_mask & g_prev_key_mask;
    g_prev_key_mask = curr_key_mask;

    if (!sel_held) {
        // Pure Playable Keyboard: both when playing AND when stopped
        if (pressed) {
            for (uint8_t k = 0; k < 27; k++) {
                if (pressed & (1u << k)) {
                    int note_val = (int)KEY_LEAD_BASE + (g_lead_octave_shift * 12) + k;
                    if (note_val < 0) note_val = 0;
                    if (note_val > 127) note_val = 127;
                    dsp_note_on_lead((uint8_t)note_val, 115, p->master_tuning);
                }
            }
        }
        if (released) {
            for (uint8_t k = 0; k < 27; k++) {
                if (released & (1u << k)) {
                    int note_val = (int)KEY_LEAD_BASE + (g_lead_octave_shift * 12) + k;
                    if (note_val < 0) note_val = 0;
                    if (note_val > 127) note_val = 127;
                    dsp_note_off_lead((uint8_t)note_val);
                }
            }
        }
    } else {
        // SEL held: Section Control, Key Selection, Cues & Form Modes
        if (pressed) {
            for (uint8_t k = 0; k < 27; k++) {
                if (pressed & (1u << k)) {
                    if (k < 12) {
                        // Zone 1: Root Key Select (Keys 0..11 -> F3..E4)
                        uint8_t target_semitone = (uint8_t)((KEY0_OFFSET + k) % 12);
                        if (!hs->is_playing) {
                            hs->root_semitone = target_semitone;
                            p->root_key = target_semitone;
                            hs->has_queued_key = 0;
                        } else if (ms_now - g_last_key_press_ms[k] <= DOUBLE_TAP_WINDOW_MS) {
                            // Double Tap: Immediate bar modulation
                            hs->root_semitone = target_semitone;
                            p->root_key = target_semitone;
                            hs->has_queued_key = 0;
                            hs->mod_next_bar = 0;
                        } else {
                            // Single Tap: Queue modulation for next chorus boundary
                            hs->queued_next_key = target_semitone;
                            hs->has_queued_key = 1;
                        }
                        g_last_key_press_ms[k] = ms_now;
                        hs->ui_dirty = 1;
                    } else if (k >= 12 && k <= 17) {
                        // Zone 2: Flow Control Cues
                        switch (k) {
                            case 12: hs->queued_cue = CUE_TO_12BAR; break;
                            case 13: hs->queued_cue = (hs->is_vamp_mode ? CUE_TO_12BAR : CUE_TO_VAMP); break;
                            case 14: hs->queued_cue = CUE_STOP_TIME; break;
                            case 15: hs->queued_cue = CUE_HOLD_CHORD; break;
                            case 16: hs->queued_cue = CUE_FORCE_TURNAROUND; break;
                            case 17: hs->manual_stab_pending = 1; break;
                        }
                        hs->ui_dirty = 1;
                    } else if (k >= 18 && k <= 22) {
                        // Zone 3: Form Variations
                        switch (k) {
                            case 18: hs->form_mode = FORM_12BAR; break;
                            case 19: hs->form_mode = FORM_QUICK_CHANGE; break;
                            case 20: hs->form_mode = FORM_145_OPENING; break;
                            case 21: hs->form_mode = FORM_8BAR; break;
                            case 22: hs->form_mode = FORM_MINOR_BLUES; break;
                        }
                        p->form_mode = hs->form_mode;
                        hs->ui_dirty = 1;
                        g_save_pending = 1;
                        g_last_param_change_ms = ms_now;
                    } else if (k >= 23 && k <= 26) {
                        // Zone 4: Outros & Endings
                        switch (k) {
                            case 23: hs->outro_type = OUTRO_STOP; break;
                            case 24: hs->outro_type = OUTRO_TAG; break;
                            case 25: hs->outro_type = OUTRO_TRIPLE; break;
                            case 26: hs->outro_type = OUTRO_CHROMATIC; break;
                        }
                        hs->ending_armed = 1;
                        p->outro_type = hs->outro_type;
                        hs->ui_dirty = 1;
                    }
                }
            }
        }
    }
}

void panel_update_leds(const HarmonyState *hs, uint8_t active_preset_idx) {
    (void)active_preset_idx;
    hal_set_btn_led(BTN_PLAY, hs->is_playing ? 1 : 0);
    hal_set_btn_led(BTN_REC, (hs->is_playing && hs->current_beat == 0 && hs->current_subdiv < 4) ? 1 : 0);

    uint8_t sel_held = (hal_get_btns_mask() & (1u << BTN_SEL)) != 0;
    uint8_t root_blink = (hs->is_playing && hs->current_beat == 0 && hs->current_subdiv < 4);

    if (hs->is_playing && !sel_held) {
        // Chord-following blues LED guide: base scale + missing target notes of the active bar's chord
        static const uint8_t MAJ_BLUES[12] = { 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0 }; // 1, 2, b3, 3, 5, 6
        static const uint8_t MIN_BLUES[12] = { 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0 }; // 1, b3, 4, b5, 5, b7
        uint8_t is_minor = (hs->form_mode == FORM_MINOR_BLUES);
        const uint8_t *base = is_minor ? MIN_BLUES : MAJ_BLUES;

        uint8_t scale[12];
        for (uint8_t i = 0; i < 12; i++) scale[i] = base[i];

        BarSlot slot = harmony_get_current_bar_slot(hs);
        ScaleDegree deg = (hs->current_beat >= 2 && slot.has_split) ? slot.secondary : slot.primary;
        if (!is_minor) {
            if (deg == DEG_IV7 || deg == DEG_iv7) {
                scale[4] = 0; scale[5] = 1; // Swap 3 -> 4 (IV7 root) so no 4-in-a-row clump
            } else if (deg == DEG_V7 || deg == DEG_v7) {
                scale[3] = 0; scale[5] = 1; // Swap b3 -> 4 (V7 b7)
                scale[9] = 0; scale[11] = 1; // Swap 6 -> 7 (V7 3rd)
            }
        }

        for (uint8_t k = 0; k < 27; k++) {
            int note_val = (int)KEY_LEAD_BASE + (g_lead_octave_shift * 12) + k;
            uint8_t note = (uint8_t)((note_val < 0) ? 0 : (note_val > 127) ? 127 : note_val);
            uint8_t interval = (uint8_t)((note + 12 - hs->root_semitone) % 12);

            hal_set_key_led_glow(k, 0);
            if (scale[interval]) {
                uint8_t pulse_off = (interval == 0 && root_blink);
                hal_set_key_led(k, pulse_off ? 0 : 1);
            } else {
                hal_set_key_led(k, 0);
            }
        }
    } else {
        // Stopped or SEL held: all keys backlit for navigation & section cues
        for (uint8_t k = 0; k < 27; k++) {
            hal_set_key_led_glow(k, 0);
            if (k < 12) {
                uint8_t semitone = (uint8_t)((KEY0_OFFSET + k) % 12);
                if (semitone == hs->root_semitone && root_blink) {
                    hal_set_key_led(k, 0);
                } else {
                    hal_set_key_led(k, 1);
                }
            } else {
                hal_set_key_led(k, 1);
            }
        }
    }
}
