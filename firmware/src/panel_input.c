#include "panel_input.h"
#include "hal_adapter.h"
#include "dsp_voices.h"
#include "blues_harmony.h"
#include "blues_arranger.h"

#define ZONE1_COUNT 12u
#define KEY0_OFFSET 53u // F3 (MIDI 53)
#define KEY_LEAD_BASE 53u // F3 (MIDI 53) - 2-octave playable solo keyboard (F3..G5)

static uint32_t g_prev_key_mask = 0;
static uint32_t g_last_key_press_ms[ZONE1_COUNT] = {0};
static uint8_t  g_save_pending = 0;
static uint32_t g_last_param_change_ms = 0;
static uint32_t g_last_tap_ms = 0;
static int8_t   g_lead_octave_shift = 0;
static uint8_t  g_save_prev_held = 0;
static uint8_t  g_save_used_for_slot = 0;

static const uint8_t WHITE_KEYS[16] = { 0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26 };

static int8_t white_key_to_slot(uint8_t k) {
    for (uint8_t s = 0; s < 16; s++) {
        if (WHITE_KEYS[s] == k) return (int8_t)s;
    }
    return -1;
}

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

static uint8_t wrap_step_u8(uint8_t cur, int32_t delta, uint8_t count) {
    int32_t v = (int32_t)cur + delta;
    while (v < 0) v += count;
    return (uint8_t)(v % count);
}

static void format_user_preset_name(char *dst, uint8_t slot_1based) {
    static const char *PFX = "USER SETTINGS ";
    int i = 0;
    while (PFX[i]) { dst[i] = PFX[i]; i++; }
    if (slot_1based >= 10) dst[i++] = (char)('0' + (slot_1based / 10));
    dst[i++] = (char)('0' + (slot_1based % 10));
    dst[i] = '\0';
}

static void compute_scale_context(const HarmonyState *hs, uint8_t *out_chord_root, const uint8_t **out_scale) {
    static const uint8_t MAJ_BLUES[12] = { 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0 }; // 1, 2, b3, 3, 5, 6
    static const uint8_t MIN_BLUES[12] = { 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 0 }; // 1, b3, 4, b5, 5, b7
    uint8_t use_minor_scale = (hs->form_mode == FORM_MINOR_BLUES);
    uint8_t chord_root = (uint8_t)(hs->root_semitone % 12);

    if (hs->is_playing) {
        BarSlot slot = harmony_get_current_bar_slot(hs);
        ScaleDegree deg = (hs->current_beat >= 2 && slot.has_split) ? slot.secondary : slot.primary;
        ScaleDegreeInfo dinfo = harmony_get_degree_info(deg);
        chord_root = (uint8_t)((hs->root_semitone + dinfo.semitone_offset) % 12);
        if (!use_minor_scale) {
            if (deg == DEG_IV7 || deg == DEG_iv7 || deg == DEG_V7 || deg == DEG_v7 ||
                deg == DEG_bVI7 || deg == DEG_bVII7 || deg == DEG_SHARP_IV_DIM7) {
                use_minor_scale = 1;
            }
        }
    }
    *out_chord_root = chord_root;
    *out_scale = use_minor_scale ? MIN_BLUES : MAJ_BLUES;
}

static uint8_t resolve_easy_lead_note(const HarmonyState *hs, uint8_t note) {
    uint8_t chord_root = 0;
    const uint8_t *scale = 0;
    compute_scale_context(hs, &chord_root, &scale);

    for (uint8_t cand = note; cand <= 127; cand++) {
        uint8_t pc = (uint8_t)(cand % 12);
        uint8_t interval = (uint8_t)((cand + 12 - (hs->root_semitone % 12)) % 12);
        if (pc == chord_root || scale[interval]) {
            return cand;
        }
    }
    return note;
}

static void save_to_preset_slot(HarmonyState *hs, PresetConfig *presets, uint8_t *active_preset_idx, uint8_t slot, uint32_t ms_now) {
    PresetConfig *src = &presets[*active_preset_idx];
    PresetConfig *dst = &presets[slot % NUM_PRESET_SLOTS];
    if (dst != src) {
        *dst = *src;
        dst->preset_id = slot % NUM_PRESET_SLOTS;
    }
    dst->magic = PRESET_MAGIC;
    dst->root_key = hs->root_semitone;
    dst->variation = hs->variation;
    dst->fullness = hs->fullness;
    dst->bass_fullness = hs->bass_fullness;
    dst->keys_fullness = hs->keys_fullness;
    dst->drums_fullness = hs->drums_fullness;
    dst->bass_variation = hs->bass_variation;
    dst->keys_variation = hs->keys_variation;
    dst->drums_variation = hs->drums_variation;
    dst->bass_style = hs->bass_style;
    dst->keys_style = hs->keys_style;
    dst->drum_pattern = hs->drum_pattern;
    dst->form_mode = (uint8_t)hs->form_mode;
    dst->opening_var = (uint8_t)hs->opening_var;
    dst->outro_type = (uint8_t)hs->outro_type;
    dst->count_in_enabled = hs->count_in_enabled;
    dst->easy_lead = hs->easy_lead;
    *active_preset_idx = slot % NUM_PRESET_SLOTS;
    format_user_preset_name(dst->name, (uint8_t)((*active_preset_idx) + 1));
    hal_blob_save(0, presets, sizeof(PresetConfig) * NUM_PRESET_SLOTS);
    GlobalSettings gs = { SETTINGS_MAGIC, *active_preset_idx, dst->master_tuning, dst->count_in_enabled, hs->easy_lead };
    hal_settings_save(&gs, sizeof(gs));
    g_save_pending = 0;
    set_toast(hs, ms_now, dst->name);
}

static void load_from_preset_slot(HarmonyState *hs, PresetConfig *presets, uint8_t *active_preset_idx, uint8_t slot, uint32_t ms_now) {
    slot %= NUM_PRESET_SLOTS;
    *active_preset_idx = slot;
    const PresetConfig *np = &presets[slot];
    hs->root_semitone = np->root_key % 12;
    hs->form_mode = (FormMode)(np->form_mode % 5);
    hs->opening_var = (OpeningVar)(np->opening_var % 4);
    hs->outro_type = (OutroType)(np->outro_type % 4);
    hs->variation = np->variation;
    hs->bass_variation = np->bass_variation;
    hs->keys_variation = np->keys_variation;
    hs->drums_variation = np->drums_variation;
    hs->fullness = np->fullness;
    hs->bass_fullness = np->bass_fullness;
    hs->keys_fullness = np->keys_fullness;
    hs->drums_fullness = np->drums_fullness;
    hs->bass_style = np->bass_style;
    hs->keys_style = np->keys_style;
    hs->drum_pattern = np->drum_pattern;
    hs->count_in_enabled = np->count_in_enabled;
    dsp_set_lead_play_style(np->lead_play_style);
    GlobalSettings gs = { SETTINGS_MAGIC, slot, np->master_tuning, np->count_in_enabled, hs->easy_lead };
    hal_settings_save(&gs, sizeof(gs));
    hs->ui_dirty = 1;
    if (np->name[0] != '\0') {
        set_toast(hs, ms_now, np->name);
    } else {
        set_toast_val(hs, ms_now, "LOADED SLOT ", (int32_t)slot + 1, "");
    }
}

static void set_master_fullness(HarmonyState *hs, PresetConfig *p, uint8_t val) {
    hs->fullness = val; hs->bass_fullness = val; hs->keys_fullness = val; hs->drums_fullness = val;
    p->fullness  = val; p->bass_fullness  = val; p->keys_fullness  = val; p->drums_fullness  = val;
}

static void set_master_variation(HarmonyState *hs, PresetConfig *p, uint8_t val) {
    hs->variation = val; hs->bass_variation = val; hs->keys_variation = val; hs->drums_variation = val;
    p->variation  = val; p->bass_variation = val; p->keys_variation = val; p->drums_variation = val;
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

static const char *INTRO_TOAST_LABELS[NUM_INTRO_MODES] = {
    "INTRO: OFF", "INTRO: AUTO", "INTRO: 1B V7", "INTRO: 2B TURN",
    "INTRO: 2B FUNK", "INTRO: 4B BAND", "INTRO: 1B CLICK"
};
static const char *OUTRO_TOAST_LABELS[NUM_OUTRO_TYPES] = {
    "OUTRO: CLASSIC SLIDE", "OUTRO: V-bV-IV-I", "OUTRO: IV-I TURN", "OUTRO: 1B QUICK", "OUTRO: STINGER"
};

static void handle_footswitch(HarmonyState *hs, uint8_t sw) {
    uint32_t ms_now = hal_millis();
    switch (sw & 3u) {
        case 0: // Switch A: Start (stopped) / Next-Measure Outro (playing) / Hard Stop (if already in outro)
            if (!hs->is_playing) {
                harmony_start_playback(hs, (GrooveStyle)hs->drum_pattern, 0);
            } else if (hs->ending_armed == ENDING_NONE) {
                hs->ending_armed = ENDING_ARMED_NEXT_BAR;
                set_toast(hs, ms_now, "OUTRO NEXT BAR");
                hs->ui_dirty = 1;
            } else {
                hs->is_playing = 0;
                hs->ending_armed = ENDING_NONE;
                hs->count_in_bars_remaining = 0;
                hs->stop_time_counter = 0;
                hs->queued_cue = CUE_NONE;
                dsp_all_notes_off();
                hs->ui_dirty = 1;
            }
            break;
        case 1: // Switch B: 1-Stomp Start WITH Style Intro (stopped) / Drum Fill or Turnaround (playing)
            if (!hs->is_playing) {
                harmony_start_playback(hs, (GrooveStyle)hs->drum_pattern, 1);
            } else if (hs->fill_pending) {
                hs->queued_cue = CUE_FORCE_TURNAROUND;
                hs->ui_dirty = 1;
            } else {
                hs->fill_pending = 1;
                hs->ui_dirty = 1;
            }
            break;
        case 2: // Switch C: Cycle Intro Mode (stopped) / Stop-Time Break (playing)
            if (!hs->is_playing) {
                hs->count_in_enabled = (uint8_t)((hs->count_in_enabled + 1) % NUM_INTRO_MODES);
                set_toast(hs, ms_now, INTRO_TOAST_LABELS[hs->count_in_enabled]);
            } else {
                hs->queued_cue = CUE_STOP_TIME;
            }
            hs->ui_dirty = 1;
            break;
        case 3: // Switch D: Cycle Outro Style (stopped) / Vamp Toggle (playing)
            if (!hs->is_playing) {
                hs->outro_type = (OutroType)(((uint8_t)hs->outro_type + 1) % NUM_OUTRO_TYPES);
                set_toast(hs, ms_now, OUTRO_TOAST_LABELS[(uint8_t)hs->outro_type]);
            } else {
                hs->queued_cue = (hs->is_vamp_mode ? CUE_TO_12BAR : CUE_TO_VAMP);
            }
            hs->ui_dirty = 1;
            break;
    }
}

void panel_handle_midi_in(HarmonyState *hs, uint8_t status, uint8_t d1, uint8_t d2) {
    if (status == 0xFA) { // Real-Time Start
        harmony_start_playback(hs, (GrooveStyle)hs->drum_pattern, 0);
        return;
    }
    if (status == 0xFC) { // Real-Time Stop
        hs->is_playing = 0;
        hs->ending_armed = ENDING_NONE;
        hs->count_in_bars_remaining = 0;
        hs->stop_time_counter = 0;
        hs->queued_cue = CUE_NONE;
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
    if (msg_type == 0xC0) { // Program Change (M-VAVE Chocolate default mode: PC 0..3)
        handle_footswitch(hs, (uint8_t)(d1 & 3u));
        return;
    }
    if (msg_type == 0x90 && d2 > 0) { // Note On (Chocolate Custom Note mode: C4/D4/E4/F4)
        if (d1 == 60)      handle_footswitch(hs, 0);
        else if (d1 == 62) handle_footswitch(hs, 1);
        else if (d1 == 64) handle_footswitch(hs, 2);
        else if (d1 == 65) handle_footswitch(hs, 3);
        return;
    }
    if (msg_type == 0xB0) { // Control Change
        if (d1 == 64) { // Sustain Pedal -> Hold Chord
            hs->hold_chord_active = (d2 >= 64) ? 1 : 0;
            hs->ui_dirty = 1;
        } else if (d2 >= 64) {
            if (d1 >= 80 && d1 <= 83)      handle_footswitch(hs, (uint8_t)(d1 - 80));
            else if (d1 >= 20 && d1 <= 23) handle_footswitch(hs, (uint8_t)(d1 - 20));
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
        if (!hs->is_playing) {
            harmony_start_playback(hs, (GrooveStyle)p->groove_style, 0);
        } else {
            hs->is_playing = 0;
            hs->ending_armed = ENDING_NONE;
            hs->count_in_bars_remaining = 0;
            hs->stop_time_counter = 0;
            hs->queued_cue = CUE_NONE;
            dsp_all_notes_off();
            hs->ui_dirty = 1;
        }
    }
    if (btn_edges & (1u << BTN_REC)) {
        if (!hs->is_playing) {
            hs->count_in_enabled = (uint8_t)((hs->count_in_enabled + 1) % NUM_INTRO_MODES);
            p->count_in_enabled = hs->count_in_enabled;
            set_toast(hs, ms_now, INTRO_TOAST_LABELS[hs->count_in_enabled]);
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else {
            if (hs->ending_armed == ENDING_NONE) {
                hs->ending_armed = ENDING_ARMED_NEXT_BAR;
                set_toast(hs, ms_now, "OUTRO NEXT BAR");
            } else if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
                hs->ending_armed = ENDING_NONE;
                set_toast(hs, ms_now, "OUTRO CANCELLED");
            }
        }
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_HOME)) {
        if (ui_screen) {
            *ui_screen = (uint8_t)((*ui_screen + 1) % 4);
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
    uint8_t save_held = (hal_get_btns_mask() & (1u << BTN_SAVE)) != 0;
    if (btn_edges & (1u << BTN_SAVE)) {
        g_save_used_for_slot = 0;
    }
    if (g_save_prev_held && !save_held && !g_save_used_for_slot) {
        // Short tap of SAVE (no white key tapped): save to active slot
        save_to_preset_slot(hs, presets, active_preset_idx, *active_preset_idx, ms_now);
    }
    g_save_prev_held = save_held;
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
        if (!hs->is_playing) {
            hs->outro_type = (OutroType)(((uint8_t)hs->outro_type + 1) % NUM_OUTRO_TYPES);
            p->outro_type = (uint8_t)hs->outro_type;
            set_toast(hs, ms_now, OUTRO_TOAST_LABELS[(uint8_t)hs->outro_type]);
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else {
            if (hs->ending_armed == ENDING_NONE) {
                hs->ending_armed = ENDING_ARMED_NEXT_BAR;
                set_toast(hs, ms_now, "OUTRO NEXT BAR");
            } else if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
                hs->ending_armed = ENDING_NONE;
                set_toast(hs, ms_now, "OUTRO CANCELLED");
            }
        }
        hs->ui_dirty = 1;
    }
    if (btn_edges & (1u << BTN_ENV)) {
        // Immediate Vamp Mode toggle on current chord
        hs->is_vamp_mode = !hs->is_vamp_mode;
        hs->vamp_bar_counter = 0;
        hs->queued_cue = CUE_NONE;
        set_toast(hs, ms_now, hs->is_vamp_mode ? "VAMP: ON" : "VAMP: OFF");
    }
    if (btn_edges & (1u << BTN_SEQ)) {
        // Trigger 4-Bar Stop-Time Break (next bar if playing)
        if (hs->is_playing) {
            if (hs->stop_time_counter > 0) {
                hs->stop_time_counter = 0;
                hs->queued_cue = CUE_NONE;
                set_toast(hs, ms_now, "STOP-TIME: OFF");
            } else {
                hs->queued_cue = CUE_STOP_TIME;
                set_toast(hs, ms_now, "STOP-TIME QUEUED");
            }
        }
    }
    if (btn_edges & (1u << BTN_LFO)) {
        if (glo_held || sel_held) {
            // Quick flip anywhere: GLO + LFO (or SEL + LFO) toggles Easy Lead Scale-Lock Mode
            hs->easy_lead = !hs->easy_lead;
            p->easy_lead = hs->easy_lead;
            GlobalSettings gs = { SETTINGS_MAGIC, *active_preset_idx, p->master_tuning, p->count_in_enabled, hs->easy_lead };
            hal_settings_save(&gs, sizeof(gs));
            set_toast(hs, ms_now, hs->easy_lead ? "EASY LEAD: ON (BEND)" : "EASY LEAD: OFF");
        } else {
            // Toggle Minor Blues Form
            if (hs->form_mode == FORM_MINOR_BLUES) {
                hs->form_mode = FORM_12BAR;
            } else {
                hs->form_mode = FORM_MINOR_BLUES;
            }
            p->form_mode = hs->form_mode;
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // 2. Rotary Encoders
    int8_t delta;

    static const char *LEAD_PLAY_LABELS[NUM_LEAD_PLAY_STYLES] = {
        "LEAD: POLY", "LEAD: LEGATO GLIDE", "LEAD: BLUES BEND", "LEAD: OCTAVE STACK"
    };

    // ENC_PRESETS: Lead Voice (0..13), Key (0..11) with SEL held, Lead Play Style (0..3) with GLO held
    if ((delta = hal_get_encoder_delta(ENC_PRESETS)) != 0) {
        if (sel_held) {
            hs->root_semitone = wrap_step_u8(hs->root_semitone, delta, 12);
            p->root_key = hs->root_semitone;
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else if (glo_held) {
            p->lead_play_style = wrap_step_u8(p->lead_play_style, delta, NUM_LEAD_PLAY_STYLES);
            dsp_set_lead_play_style(p->lead_play_style);
            set_toast(hs, ms_now, LEAD_PLAY_LABELS[p->lead_play_style]);
            g_save_pending = 1;
            g_last_param_change_ms = ms_now;
        } else {
            p->lead_instrument = wrap_step_u8(p->lead_instrument, delta, NUM_LEAD_INSTRUMENTS);
        }
        hs->ui_dirty = 1;
    }

    static const char *TUNE_LABELS[3] = { "TUNE: 440 HZ", "TUNE: 432 HZ", "TUNE: 442 HZ" };
    static const char *BASS_LABELS[NUM_BASS_INSTRUMENTS] = {
        "BASS: P-BASS", "BASS: UPRIGHT", "BASS: MUTED BOOGIE", "BASS: MOOG FUNK",
        "BASS: 60S JAZZ", "BASS: FLATWOUND", "BASS: AMPEG TUBE", "BASS: SUB OCTAVE",
        "BASS: LATELY FM", "BASS: SLAP POP", "BASS: HOFNER", "BASS: ACID 303",
        "BASS: TUBA", "BASS: PICK ROCK"
    };

    // ENC_ALGORITHM: Backing Chord Sound (0..17)
    if ((delta = hal_get_encoder_delta(ENC_ALGORITHM)) != 0) {
        p->ep_tone_algo = wrap_step_u8(p->ep_tone_algo, delta, NUM_KEYS_INSTRUMENTS);
        hs->ui_dirty = 1;
    }

    // ENC_SELECT: Groove Preset (0..13), or Drum Kit (0..11) with GLO held
    if ((delta = hal_get_encoder_delta(ENC_SELECT)) != 0) {
        if (glo_held) {
            p->drum_kit = wrap_step_u8(p->drum_kit, delta, NUM_DRUM_KITS);
        } else {
            uint8_t ng = wrap_step_u8(p->groove_style, delta, NUM_DRUM_PATTERNS);
            arranger_apply_groove_defaults(hs, p, (GrooveStyle)ng);
        }
        hs->ui_dirty = 1;
    }

    uint8_t scr = ui_screen ? *ui_screen : 0;
    uint8_t is_mixer = (scr == 1);
    uint8_t is_style = (scr == 2);
    uint8_t is_feel  = (scr == 3);

    // KNOB 1: Lead Sheet (BPM / GLO:Swing) | Mixer (Bass Vol / GLO:Reverb) | Style (Bass Line / GLO:Bass Sound) | Feel (Bass Ful / GLO:Bass Spc)
    if ((delta = hal_get_encoder_delta(ENC_KNOB1)) != 0) {
        if (is_feel) {
            if (glo_held) { hs->bass_variation = clamp_step_u8(hs->bass_variation, delta * 5, 0, 100); p->bass_variation = hs->bass_variation; }
            else          { hs->bass_fullness = clamp_step_u8(hs->bass_fullness, delta * 5, 0, 100); p->bass_fullness = hs->bass_fullness; }
        } else if (is_style) {
            if (glo_held) {
                p->bass_instrument = wrap_step_u8(p->bass_instrument, delta, NUM_BASS_INSTRUMENTS);
            } else {
                hs->bass_style = wrap_step_u8(hs->bass_style, delta, NUM_BASS_STYLES);
                p->bass_style = hs->bass_style;
            }
        } else if (is_mixer) {
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

    // KNOB 2: Lead Sheet (Master Spice / GLO:Tuning) | Mixer (Keys Vol / GLO:Master Spice) | Style (Keys Rhy / GLO:Chord Sound) | Feel (Keys Ful / GLO:Keys Spc)
    if ((delta = hal_get_encoder_delta(ENC_KNOB2)) != 0) {
        if (is_feel) {
            if (glo_held) {
                hs->keys_variation = clamp_step_u8(hs->keys_variation, delta * 5, 0, 100);
                p->keys_variation = hs->keys_variation;
            } else {
                hs->keys_fullness = clamp_step_u8(hs->keys_fullness, delta * 5, 0, 100);
                p->keys_fullness = hs->keys_fullness;
            }
        } else if (is_style) {
            if (glo_held) {
                p->drum_kit = wrap_step_u8(p->drum_kit, delta, NUM_DRUM_KITS);
            } else {
                hs->keys_style = wrap_step_u8(hs->keys_style, delta, NUM_KEYS_STYLES);
                p->keys_style = hs->keys_style;
            }
        } else if (is_mixer) {
            if (glo_held) {
                p->drive_level = clamp_step_u8(p->drive_level, delta * 4, 0, 127);
            } else {
                p->keys_vol = clamp_step_u8(p->keys_vol, delta * 4, 0, 127);
            }
        } else {
            if (glo_held) {
                p->master_tuning = wrap_step_u8(p->master_tuning, delta, 3);
                set_toast(hs, ms_now, TUNE_LABELS[p->master_tuning]);
            } else {
                set_master_variation(hs, p, clamp_step_u8(hs->variation, delta * 5, 0, 100));
            }
        }
        hs->ui_dirty = 1;
        g_save_pending = 1;
        g_last_param_change_ms = ms_now;
    }

    // KNOB 3: Lead Sheet (Bass Vol / GLO:Bass Sound) | Mixer (Drums Vol / GLO:Swing) | Style (Drum Beat / GLO:Drum Kit) | Feel (Drums Ful / GLO:Drums Spc)
    if ((delta = hal_get_encoder_delta(ENC_KNOB3)) != 0) {
        if (is_feel) {
            if (glo_held) { hs->drums_variation = clamp_step_u8(hs->drums_variation, delta * 5, 0, 100); p->drums_variation = hs->drums_variation; }
            else          { hs->drums_fullness = clamp_step_u8(hs->drums_fullness, delta * 5, 0, 100); p->drums_fullness = hs->drums_fullness; }
        } else if (is_style) {
            if (glo_held) {
                hs->count_in_enabled = wrap_step_u8(hs->count_in_enabled, delta, NUM_INTRO_MODES);
                p->count_in_enabled = hs->count_in_enabled;
                set_toast(hs, ms_now, INTRO_TOAST_LABELS[hs->count_in_enabled]);
            } else {
                hs->drum_pattern = wrap_step_u8(hs->drum_pattern, delta, NUM_DRUM_PATTERNS);
                p->drum_pattern = hs->drum_pattern;
            }
        } else if (is_mixer) {
            if (glo_held) p->swing     = clamp_step_u8(p->swing,     delta * 4, 0, 100);
            else          p->drums_vol = clamp_step_u8(p->drums_vol, delta * 4, 0, 127);
        } else {
            if (glo_held) {
                p->bass_instrument = wrap_step_u8(p->bass_instrument, delta, NUM_BASS_INSTRUMENTS);
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

    // KNOB 4: Lead Sheet (Keys Vol / GLO:Reverb) | Mixer (Lead Vol / GLO:Master Ful) | Style (Form / GLO:Tuning) | Feel (Master Ful / GLO:Master Spc)
    if ((delta = hal_get_encoder_delta(ENC_KNOB4)) != 0) {
        if (is_feel) {
            if (glo_held) set_master_variation(hs, p, clamp_step_u8(hs->variation, delta * 5, 0, 100));
            else          set_master_fullness(hs, p, clamp_step_u8(hs->fullness, delta * 5, 0, 100));
        } else if (is_style) {
            if (glo_held) {
                hs->outro_type = (OutroType)wrap_step_u8((uint8_t)hs->outro_type, delta, NUM_OUTRO_TYPES);
                p->outro_type = (uint8_t)hs->outro_type;
                set_toast(hs, ms_now, OUTRO_TOAST_LABELS[(uint8_t)hs->outro_type]);
            } else {
                hs->form_mode = (FormMode)wrap_step_u8((uint8_t)hs->form_mode, delta, 5);
                p->form_mode = hs->form_mode;
            }
        } else if (is_mixer) {
            if (sel_held) {
                hs->easy_lead = !hs->easy_lead;
                p->easy_lead = hs->easy_lead;
                GlobalSettings gs = { SETTINGS_MAGIC, *active_preset_idx, p->master_tuning, p->count_in_enabled, hs->easy_lead };
                hal_settings_save(&gs, sizeof(gs));
                set_toast(hs, ms_now, hs->easy_lead ? "EASY LEAD: ON (BEND)" : "EASY LEAD: OFF");
            } else if (glo_held) {
                p->lead_play_style = wrap_step_u8(p->lead_play_style, delta, NUM_LEAD_PLAY_STYLES);
                dsp_set_lead_play_style(p->lead_play_style);
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
    if (curr_key_mask == 0 && g_prev_key_mask != 0) {
        dsp_note_off_lead(0xFF); // Guarantee all lead voices release when no keys are held
    }
    g_prev_key_mask = curr_key_mask;

    if (save_held) {
        // SAVE held + white key: save current setup into preset slot 0..15
        if (pressed) {
            for (uint8_t k = 0; k < 27; k++) {
                if (pressed & (1u << k)) {
                    int8_t slot = white_key_to_slot(k);
                    if (slot >= 0) {
                        save_to_preset_slot(hs, presets, active_preset_idx, (uint8_t)slot, ms_now);
                        g_save_used_for_slot = 1;
                    }
                }
            }
        }
    } else if (glo_held && !sel_held) {
        // GLO held + white key: load preset slot 0..15
        if (pressed) {
            for (uint8_t k = 0; k < 27; k++) {
                if (pressed & (1u << k)) {
                    int8_t slot = white_key_to_slot(k);
                    if (slot >= 0) {
                        load_from_preset_slot(hs, presets, active_preset_idx, (uint8_t)slot, ms_now);
                    }
                }
            }
        }
    } else if (!sel_held) {
        // Pure Playable Keyboard: both when playing AND when stopped
        if (pressed) {
            for (uint8_t k = 0; k < 27; k++) {
                if (pressed & (1u << k)) {
                    int note_val = (int)KEY_LEAD_BASE + (g_lead_octave_shift * 12) + k;
                    if (note_val < 0) note_val = 0;
                    if (note_val > 127) note_val = 127;
                    uint8_t key_note = (uint8_t)note_val;
                    uint8_t target_note = hs->easy_lead ? resolve_easy_lead_note(hs, key_note) : key_note;
                    dsp_note_on_lead_ex(key_note, target_note, 115, p->master_tuning);
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
                        // Zone 4: Outros & Endings (tap again to disarm)
                        OutroType chosen = OUTRO_CLASSIC_SLIDE;
                        switch (k) {
                            case 23: chosen = OUTRO_CLASSIC_SLIDE; break;
                            case 24: chosen = OUTRO_V_bV_IV_I;     break;
                            case 25: chosen = OUTRO_IV_I_TURN;     break;
                            case 26: chosen = OUTRO_QUICK_1BAR;    break;
                        }
                        if (hs->ending_armed == ENDING_ARMED_NEXT_BAR && hs->outro_type == chosen) {
                            hs->ending_armed = ENDING_NONE;
                            set_toast(hs, ms_now, "OUTRO CANCELLED");
                        } else {
                            hs->outro_type = chosen;
                            p->outro_type = (uint8_t)hs->outro_type;
                            if (hs->is_playing) {
                                hs->ending_armed = ENDING_ARMED_NEXT_BAR;
                                set_toast(hs, ms_now, "OUTRO NEXT BAR");
                            } else {
                                set_toast(hs, ms_now, OUTRO_TOAST_LABELS[(uint8_t)chosen]);
                            }
                        }
                        hs->ui_dirty = 1;
                    }
                }
            }
        }
    }
}

void panel_update_leds(const HarmonyState *hs, uint8_t active_preset_idx) {
    (void)active_preset_idx;
    uint8_t play_led = hs->is_playing ? 1 : 0;
    if (hs->is_playing && hs->ending_armed != ENDING_NONE) {
        play_led = (hs->current_subdiv < 12) ? 1 : 0; // Blink PLAY LED when an ending is queued/active
    }
    hal_set_btn_led(BTN_PLAY, play_led);
    uint8_t rec_led = 0;
    if (!hs->is_playing) {
        rec_led = (hs->count_in_enabled != INTRO_OFF) ? 1 : 0;
    } else if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
        rec_led = (hs->current_subdiv < 12) ? 1 : 0;
    } else {
        rec_led = (hs->current_beat == 0 && hs->current_subdiv < 4) ? 1 : 0;
    }
    hal_set_btn_led(BTN_REC, rec_led);

    uint8_t sel_held = (hal_get_btns_mask() & (1u << BTN_SEL)) != 0;
    uint8_t save_held = (hal_get_btns_mask() & (1u << BTN_SAVE)) != 0;
    uint8_t glo_held = hal_get_glo_held();
    uint8_t root_blink = (hs->is_playing && hs->current_beat == 0 && hs->current_subdiv < 4);

    if (save_held || (glo_held && !sel_held)) {
        // Flash the 16 white keys to show preset save/load slots; active slot stays solid bright
        uint8_t flash_on = (uint8_t)((hal_millis() / 160u) & 1u);
        uint8_t active_slot = (uint8_t)(active_preset_idx % 16);
        for (uint8_t k = 0; k < 27; k++) {
            hal_set_key_led_glow(k, 0);
            int8_t slot = white_key_to_slot(k);
            if (slot >= 0) {
                uint8_t on = ((uint8_t)slot == active_slot) ? 1u : flash_on;
                hal_set_key_led(k, on);
            } else {
                hal_set_key_led(k, 0);
            }
        }
    } else if (!sel_held) {
        // Classic blues phrasing LED guide (both when playing AND when stopped):
        // Current chord root = FULL WHITE, other scale notes = HALF WHITE (glow), non-scale = OFF.
        uint8_t chord_root = 0;
        const uint8_t *scale = 0;
        compute_scale_context(hs, &chord_root, &scale);

        for (uint8_t k = 0; k < 27; k++) {
            int note_val = (int)KEY_LEAD_BASE + (g_lead_octave_shift * 12) + k;
            uint8_t note = (uint8_t)((note_val < 0) ? 0 : (note_val > 127) ? 127 : note_val);
            uint8_t note_pc = (uint8_t)(note % 12);
            uint8_t interval = (uint8_t)((note + 12 - (hs->root_semitone % 12)) % 12);

            if (note_pc == chord_root) {
                hal_set_key_led(k, root_blink ? 0 : 1);
                hal_set_key_led_glow(k, 0);
            } else if (scale[interval]) {
                hal_set_key_led(k, 0);
                hal_set_key_led_glow(k, 1);
            } else {
                hal_set_key_led(k, 0);
                hal_set_key_led_glow(k, 0);
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
