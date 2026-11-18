#include "blues_harmony.h"

static const ScaleDegreeInfo G_DEGREE_INFO_TBL[12] = {
    { DEG_I7,            CHORD_DOM7, 0 },
    { DEG_IV7,           CHORD_DOM7, 5 },
    { DEG_V7,            CHORD_DOM7, 7 },
    { DEG_i7,            CHORD_MIN7, 0 },
    { DEG_iv7,           CHORD_MIN7, 5 },
    { DEG_v7,            CHORD_MIN7, 7 },
    { DEG_ii7,           CHORD_MIN7, 2 },
    { DEG_vi7,           CHORD_MIN7, 9 },
    { DEG_VI7,           CHORD_DOM7, 9 },
    { DEG_bVI7,          CHORD_DOM7, 8 },
    { DEG_bVII7,         CHORD_DOM7, 10 },
    { DEG_SHARP_IV_DIM7, CHORD_DIM7, 6 }
};

ScaleDegreeInfo harmony_get_degree_info(ScaleDegree deg) {
    if ((int)deg >= 0 && (int)deg < 12) return G_DEGREE_INFO_TBL[(int)deg];
    return G_DEGREE_INFO_TBL[0];
}

static const BarSlot FORM_12BAR_STD[12] = {
    {DEG_I7, DEG_I7, 0},   {DEG_I7, DEG_I7, 0},   {DEG_I7, DEG_I7, 0}, {DEG_I7, DEG_I7, 0},
    {DEG_IV7, DEG_IV7, 0}, {DEG_IV7, DEG_IV7, 0}, {DEG_I7, DEG_I7, 0}, {DEG_I7, DEG_I7, 0},
    {DEG_V7, DEG_V7, 0},   {DEG_IV7, DEG_IV7, 0}, {DEG_I7, DEG_I7, 0}, {DEG_V7, DEG_V7, 0}
};

static const BarSlot FORM_8BAR_GRID[8] = {
    {DEG_I7, DEG_I7, 0}, {DEG_V7, DEG_V7, 0},  {DEG_IV7, DEG_IV7, 0}, {DEG_IV7, DEG_IV7, 0},
    {DEG_I7, DEG_I7, 0}, {DEG_V7, DEG_V7, 0},  {DEG_I7, DEG_I7, 0},   {DEG_V7, DEG_V7, 0}
};

static const BarSlot FORM_MINOR_BLUES_GRID[12] = {
    {DEG_i7, DEG_i7, 0},   {DEG_i7, DEG_i7, 0},   {DEG_i7, DEG_i7, 0}, {DEG_i7, DEG_i7, 0},
    {DEG_iv7, DEG_iv7, 0}, {DEG_iv7, DEG_iv7, 0}, {DEG_i7, DEG_i7, 0}, {DEG_i7, DEG_i7, 0},
    {DEG_bVI7, DEG_bVI7, 0}, {DEG_V7, DEG_V7, 0}, {DEG_i7, DEG_i7, 0}, {DEG_V7, DEG_V7, 0}
};

void harmony_init(HarmonyState *hs) {
    hs->root_semitone = 4; // E Default
    hs->form_mode = FORM_12BAR;
    hs->opening_var = VAR_FLAT_FOUR;
    hs->outro_type = OUTRO_CLASSIC_SLIDE;
    hs->current_bar = 0;
    hs->current_beat = 0;
    hs->current_subdiv = 0;
    hs->queued_cue = CUE_NONE;
    hs->variation = 50;
    hs->fullness = 65;
    hs->bass_fullness = 65;
    hs->keys_fullness = 65;
    hs->drums_fullness = 65;
    hs->bass_variation = 50;
    hs->keys_variation = 50;
    hs->drums_variation = 50;
    hs->bass_style = BASS_STYLE_WALKING;
    hs->keys_style = KEYS_STYLE_CHARLESTON;
    hs->drum_pattern = 0;
    hs->easy_lead = 0;
    hs->heat_level = 3;
    hs->vis_bass_pulse = 0;
    hs->vis_keys_pulse = 0;
    hs->vis_drum_pulse = 0;
    hs->toast_msg[0] = '\0';
    hs->toast_until_ms = 0;
    hs->is_vamp_mode = 0;
    hs->vamp_bar_counter = 0;
    hs->stop_time_counter = 0;
    hs->ending_armed = ENDING_NONE;
    hs->queued_next_key = 0;
    hs->has_queued_key = 0;
    hs->mod_next_bar = 0;
    hs->manual_stab_pending = 0;
    hs->fill_pending = 0;
    hs->count_in_enabled = INTRO_AUTO;
    hs->count_in_bars_remaining = 0;
    hs->intro_total_bars = 0;
    hs->active_intro_style = INTRO_OFF;
    hs->hold_chord_active = 0;
    hs->is_playing = 0; // Wait for BTN_PLAY
    hs->ui_dirty = 1;
    hs->ext_clock_active = 0;
    hs->ext_clock_ticks_pending = 0;
    hs->ext_clock_timeout_ms = 0;
    hs->cpu_load_pct = 0;
    hs->chorus_count = 0;
}

void harmony_set_heat_level(HarmonyState *hs, PresetConfig *p, uint8_t level, uint8_t trigger_fill) {
    static const uint8_t HEAT_FULLNESS[5] = { 25, 45, 65, 80, 100 };
    static const uint8_t HEAT_VAR[5]      = { 15, 30, 50, 75, 100 };
    if (level < 1) level = 1;
    if (level > 5) level = 5;

    uint8_t f = HEAT_FULLNESS[level - 1];
    uint8_t v = HEAT_VAR[level - 1];
    hs->heat_level = level;
    hs->fullness = f;
    hs->bass_fullness = f;
    hs->keys_fullness = f;
    hs->drums_fullness = f;
    hs->variation = v;
    hs->bass_variation = v;
    hs->keys_variation = v;
    hs->drums_variation = v;

    if (p) {
        p->heat_level = level;
        p->fullness = f;
        p->bass_fullness = f;
        p->keys_fullness = f;
        p->drums_fullness = f;
        p->variation = v;
        p->bass_variation = v;
        p->keys_variation = v;
        p->drums_variation = v;
    }

    if (trigger_fill && hs->is_playing) {
        hs->fill_pending = 1;
    }
    hs->ui_dirty = 1;
}

IntroMode harmony_resolve_intro_style(IntroMode mode, GrooveStyle groove) {
    if (mode != INTRO_AUTO) return mode;
    switch (groove) {
        case GROOVE_STRAIGHT_ROCK:
        case GROOVE_BOOGIE_WOOGIE:
        case GROOVE_TRAIN_BEAT:
        case GROOVE_BO_DIDDLEY:
        case GROOVE_DELTA_STOMP:
        case GROOVE_COUNTRY_TWANG:
            return INTRO_V7_1BAR;
        case GROOVE_SECOND_LINE:
        case GROOVE_MEMPHIS_SOUL:
        case GROOVE_RHUMBA_BLUES:
        case GROOVE_POCKET_FUNK:
        case GROOVE_NOLA_FUNK:
        case GROOVE_BLUES_FUNK:
        case GROOVE_SLOW_NEO_FUNK:
            return INTRO_FUNK_2BAR;
        default:
            return INTRO_TURNAROUND_2BAR;
    }
}

uint8_t harmony_intro_bar_count(IntroMode concrete_mode) {
    switch (concrete_mode) {
        case INTRO_V7_1BAR:
        case INTRO_CLICK_1BAR:
            return 1;
        case INTRO_TURNAROUND_2BAR:
        case INTRO_FUNK_2BAR:
            return 2;
        case INTRO_BAND_4BAR:
            return 4;
        default:
            return 0;
    }
}

void harmony_start_playback(HarmonyState *hs, GrooveStyle groove, uint8_t force_intro) {
    hs->is_playing = 1;
    hs->current_bar = 0;
    hs->current_beat = 0;
    hs->current_subdiv = 0;
    hs->ending_armed = ENDING_NONE;
    hs->stop_time_counter = 0;
    hs->queued_cue = CUE_NONE;

    IntroMode mode = (IntroMode)(hs->count_in_enabled % NUM_INTRO_MODES);
    if (force_intro && mode == INTRO_OFF) {
        mode = INTRO_AUTO;
    }
    IntroMode resolved = harmony_resolve_intro_style(mode, groove);
    uint8_t bars = harmony_intro_bar_count(resolved);
    hs->active_intro_style = resolved;
    hs->count_in_bars_remaining = bars;
    hs->intro_total_bars = bars;
    hs->ui_dirty = 1;
}

BarSlot harmony_get_current_bar_slot(const HarmonyState *hs) {
    if (hs->is_vamp_mode) {
        BarSlot slot;
        slot.primary = (hs->vamp_bar_counter < 2) ? DEG_I7 : DEG_IV7;
        slot.secondary = slot.primary;
        slot.has_split = 0;
        return slot;
    }

    ScaleDegree root_deg = (hs->form_mode == FORM_MINOR_BLUES) ? DEG_i7 : DEG_I7;
    ScaleDegree four_deg = (hs->form_mode == FORM_MINOR_BLUES) ? DEG_iv7 : DEG_IV7;

    if (hs->ending_armed == ENDING_STINGER) {
        BarSlot slot = { root_deg, root_deg, 0 };
        return slot;
    }

    if (hs->ending_armed == ENDING_BAR_1 || hs->ending_armed == ENDING_BAR_2) {
        BarSlot slot;
        uint8_t outro_step = (hs->ending_armed == ENDING_BAR_1) ? 0 : 1;
        if (hs->outro_type == OUTRO_V_bV_IV_I) {
            if (outro_step == 0) {
                slot.primary = DEG_V7;
                slot.secondary = four_deg;
                slot.has_split = 1;
            } else {
                slot.primary = root_deg;
                slot.secondary = DEG_V7;
                slot.has_split = 1;
            }
        } else if (hs->outro_type == OUTRO_IV_I_TURN) {
            if (outro_step == 0) {
                slot.primary = four_deg;
                slot.secondary = DEG_SHARP_IV_DIM7;
                slot.has_split = 1;
            } else {
                slot.primary = DEG_ii7;
                slot.secondary = DEG_V7;
                slot.has_split = 1;
            }
        } else if (hs->outro_type == OUTRO_QUICK_1BAR) {
            slot.primary = DEG_V7;
            slot.secondary = four_deg;
            slot.has_split = 1;
        } else { // OUTRO_CLASSIC_SLIDE
            if (outro_step == 0) {
                slot.primary = root_deg;
                slot.secondary = DEG_SHARP_IV_DIM7;
                slot.has_split = 1;
            } else {
                slot.primary = root_deg;
                slot.secondary = DEG_V7;
                slot.has_split = 1;
            }
        }
        return slot;
    }

    if (hs->form_mode == FORM_8BAR) {
        return FORM_8BAR_GRID[hs->current_bar % 8];
    }
    if (hs->form_mode == FORM_MINOR_BLUES) {
        return FORM_MINOR_BLUES_GRID[hs->current_bar % 12];
    }
    if (hs->form_mode == FORM_I_IV_VAMP) {
        BarSlot slot;
        slot.primary = (hs->current_bar % 2 == 0) ? DEG_I7 : DEG_IV7;
        slot.secondary = slot.primary;
        slot.has_split = 0;
        return slot;
    }

    // FORM_12BAR and standard blues variations
    BarSlot slot = FORM_12BAR_STD[hs->current_bar % 12];

    // Opening variations apply strictly to first chorus (intro)
    if (hs->chorus_count == 0 && hs->current_bar < 4) {
        if (hs->opening_var == VAR_PICKUP_INTRO) {
            static const BarSlot INTRO_GRID[4] = {
                {DEG_V7, DEG_V7, 0}, {DEG_IV7, DEG_IV7, 0}, {DEG_I7, DEG_I7, 0}, {DEG_V7, DEG_V7, 0}
            };
            return INTRO_GRID[hs->current_bar];
        }
        if (hs->opening_var == VAR_DIM_WALKUP) {
            if (hs->current_bar == 0)      { slot.primary = DEG_I7; slot.secondary = DEG_I7; slot.has_split = 0; }
            else if (hs->current_bar == 1) { slot.primary = DEG_IV7; slot.secondary = DEG_IV7; slot.has_split = 0; }
            else if (hs->current_bar == 2) { slot.primary = DEG_SHARP_IV_DIM7; slot.secondary = DEG_SHARP_IV_DIM7; slot.has_split = 0; }
            else if (hs->current_bar == 3) { slot.primary = DEG_I7; slot.secondary = DEG_I7; slot.has_split = 0; }
            return slot;
        }
        if (hs->opening_var == VAR_QUICK_CHANGE && hs->current_bar == 1) {
            slot.primary = DEG_IV7; slot.secondary = DEG_IV7; slot.has_split = 0;
            return slot;
        }
        if (hs->opening_var == VAR_145_BOUNCE && hs->current_bar == 1) {
            slot.primary = DEG_IV7; slot.secondary = DEG_V7; slot.has_split = 1;
            return slot;
        }
    }

    // Standard progression modifications & Variation alterations
    if (hs->current_bar == 1) {
        if (hs->form_mode == FORM_QUICK_CHANGE || hs->variation > 25) {
            slot.primary = DEG_IV7; slot.secondary = DEG_IV7; slot.has_split = 0;
        } else if (hs->form_mode == FORM_145_OPENING) {
            slot.primary = DEG_IV7; slot.secondary = DEG_V7; slot.has_split = 1;
        }
    }
    if (hs->current_bar == 5 && hs->variation > 50) {
        slot.primary = DEG_IV7; slot.secondary = DEG_SHARP_IV_DIM7; slot.has_split = 1;
    }
    if (hs->current_bar == 9 && hs->variation > 75) {
        slot.primary = DEG_I7; slot.secondary = DEG_VI7; slot.has_split = 1;
    }
    if (hs->current_bar == 10 && hs->variation > 75) {
        slot.primary = DEG_bVI7; slot.secondary = DEG_V7; slot.has_split = 1;
    }
    if (hs->current_bar == 11 && hs->variation > 40) {
        slot.primary = (hs->variation > 75) ? DEG_bVI7 : DEG_I7;
        slot.secondary = DEG_V7;
        slot.has_split = 1;
    }
    return slot;
}

BarSlot harmony_get_next_bar_slot(const HarmonyState *hs) {
    HarmonyState next_hs = *hs;
    next_hs.current_bar++;
    uint8_t max_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
    if (next_hs.current_bar >= max_bars) {
        next_hs.current_bar = 0;
        next_hs.chorus_count++;
    }
    if (hs->is_vamp_mode) {
        next_hs.vamp_bar_counter = (hs->vamp_bar_counter + 1) & 3;
    }
    return harmony_get_current_bar_slot(&next_hs);
}

BarSlot harmony_get_grid_slot(const HarmonyState *hs, uint8_t bar_idx) {
    HarmonyState tmp_hs = *hs;
    tmp_hs.current_bar = bar_idx;
    return harmony_get_current_bar_slot(&tmp_hs);
}

void seq_advance_bar(HarmonyState *hs) {
    hs->ui_dirty = 1;

    // Double-tap immediate modulation on next bar
    if (hs->has_queued_key && hs->mod_next_bar) {
        hs->root_semitone = hs->queued_next_key;
        hs->has_queued_key = 0;
        hs->mod_next_bar = 0;
    }

    uint8_t cue_was_stop_time = 0;
    if (hs->queued_cue != CUE_NONE) {
        CueAction cue = hs->queued_cue;
        hs->queued_cue = CUE_NONE;
        switch (cue) {
            case CUE_TO_VAMP:
                hs->is_vamp_mode = 1;
                hs->vamp_bar_counter = 0;
                return;
            case CUE_TO_12BAR:
                hs->is_vamp_mode = 0;
                hs->current_bar = 0;
                hs->chorus_count++;
                return;
            case CUE_HOLD_CHORD:
                hs->hold_chord_active ^= 1;
                break;
            case CUE_FORCE_TURNAROUND:
                hs->current_bar = (hs->form_mode == FORM_8BAR) ? 6 : 10;
                return;
            case CUE_STOP_TIME:
                hs->stop_time_counter = 4;
                cue_was_stop_time = 1;
                break;
            case CUE_ENDING_TAG:
                hs->ending_armed = ENDING_ARMED_NEXT_BAR;
                break;
            case CUE_DRUM_FILL:
                hs->fill_pending = 1;
                break;
            default:
                break;
        }
    }

    // Any-bar Next-Measure Outro State Machine
    if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
        hs->is_vamp_mode = 0;
        hs->hold_chord_active = 0;
        hs->stop_time_counter = 0;
        hs->count_in_bars_remaining = 0;
        hs->ending_armed = (hs->outro_type == OUTRO_STINGER_ONLY) ? ENDING_STINGER :
                           (hs->outro_type == OUTRO_QUICK_1BAR) ? ENDING_BAR_2 : ENDING_BAR_1;
        return;
    } else if (hs->ending_armed == ENDING_BAR_1) {
        hs->ending_armed = ENDING_BAR_2;
        return;
    } else if (hs->ending_armed == ENDING_BAR_2) {
        hs->ending_armed = ENDING_STINGER;
        return;
    } else if (hs->ending_armed == ENDING_STINGER) {
        hs->is_playing = 0;
        hs->ending_armed = ENDING_NONE;
        hs->current_bar = 0;
        return;
    }

    if (hs->hold_chord_active) {
        return;
    }

    if (hs->count_in_bars_remaining > 0) {
        hs->count_in_bars_remaining--;
        return;
    }

    if (hs->is_vamp_mode) {
        hs->vamp_bar_counter = (hs->vamp_bar_counter + 1) & 3;
        return;
    }

    if (hs->stop_time_counter > 0 && !cue_was_stop_time) {
        hs->stop_time_counter--;
    }

    hs->current_bar++;
    uint8_t max_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
    if (hs->current_bar >= max_bars) {
        hs->current_bar = 0;
        hs->chorus_count++;
        // Single-tap key modulation on chorus boundary
        if (hs->has_queued_key) {
            hs->root_semitone = hs->queued_next_key;
            hs->has_queued_key = 0;
            hs->mod_next_bar = 0;
        }
    }
}

void harmony_tick_subdivision(HarmonyState *hs) {
    if (!hs->is_playing) return;
    hs->current_subdiv++;
    if (hs->current_subdiv >= 24) {
        hs->current_subdiv = 0;
        hs->current_beat++;
        hs->ui_dirty = 1;
        if (hs->current_beat >= 4) {
            hs->current_beat = 0;
            seq_advance_bar(hs);
        }
    }
}
