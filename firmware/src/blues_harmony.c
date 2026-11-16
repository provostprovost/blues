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
    hs->outro_type = OUTRO_TAG;
    hs->current_bar = 0;
    hs->current_beat = 0;
    hs->current_subdiv = 0;
    hs->queued_cue = CUE_NONE;
    hs->spice_level = 50;
    hs->fullness = 65;
    hs->bass_style = BASS_STYLE_WALKING;
    hs->keys_style = KEYS_STYLE_CHARLESTON;
    hs->drum_pattern = 0;
    hs->toast_msg[0] = '\0';
    hs->toast_until_ms = 0;
    hs->is_vamp_mode = 0;
    hs->vamp_bar_counter = 0;
    hs->stop_time_counter = 0;
    hs->ending_armed = 0;
    hs->queued_next_key = 0;
    hs->has_queued_key = 0;
    hs->mod_next_bar = 0;
    hs->manual_stab_pending = 0;
    hs->fill_pending = 0;
    hs->count_in_enabled = 0;
    hs->count_in_bars_remaining = 0;
    hs->hold_chord_active = 0;
    hs->is_playing = 0; // Wait for BTN_PLAY
    hs->ui_dirty = 1;
    hs->ext_clock_active = 0;
    hs->ext_clock_ticks_pending = 0;
    hs->ext_clock_timeout_ms = 0;
    hs->cpu_load_pct = 0;
    hs->chorus_count = 0;
}

BarSlot harmony_get_current_bar_slot(const HarmonyState *hs) {
    if (hs->is_vamp_mode) {
        BarSlot slot;
        slot.primary = (hs->vamp_bar_counter < 2) ? DEG_I7 : DEG_IV7;
        slot.secondary = slot.primary;
        slot.has_split = 0;
        return slot;
    }

    uint8_t outro_bar_thresh = (hs->form_mode == FORM_8BAR) ? 6 : 10;
    if (hs->ending_armed && hs->current_bar >= outro_bar_thresh) {
        BarSlot slot;
        uint8_t outro_step = hs->current_bar - outro_bar_thresh;
        if (hs->outro_type == OUTRO_CHROMATIC) {
            if (outro_step == 0) {
                slot.primary = DEG_bVI7;
                slot.secondary = DEG_V7;
                slot.has_split = 1;
            } else {
                slot.primary = DEG_I7;
                slot.secondary = DEG_I7;
                slot.has_split = 0;
            }
        } else if (hs->outro_type == OUTRO_TRIPLE) {
            if (outro_step == 0) {
                slot.primary = DEG_IV7;
                slot.secondary = DEG_IV7;
                slot.has_split = 0;
            } else {
                slot.primary = DEG_V7;
                slot.secondary = DEG_I7;
                slot.has_split = 1;
            }
        } else if (hs->outro_type == OUTRO_STOP) {
            slot.primary = DEG_I7;
            slot.secondary = DEG_I7;
            slot.has_split = 0;
        } else { // OUTRO_TAG
            if (outro_step == 0) {
                slot.primary = DEG_I7;
                slot.secondary = DEG_IV7;
                slot.has_split = 1;
            } else {
                slot.primary = DEG_I7;
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

    // Standard progression modifications & Spice alterations
    if (hs->current_bar == 1) {
        if (hs->form_mode == FORM_QUICK_CHANGE || hs->spice_level > 25) {
            slot.primary = DEG_IV7; slot.secondary = DEG_IV7; slot.has_split = 0;
        } else if (hs->form_mode == FORM_145_OPENING) {
            slot.primary = DEG_IV7; slot.secondary = DEG_V7; slot.has_split = 1;
        }
    }
    if (hs->current_bar == 5 && hs->spice_level > 50) {
        slot.primary = DEG_IV7; slot.secondary = DEG_SHARP_IV_DIM7; slot.has_split = 1;
    }
    if (hs->current_bar == 9 && hs->spice_level > 75) {
        slot.primary = DEG_I7; slot.secondary = DEG_VI7; slot.has_split = 1;
    }
    if (hs->current_bar == 10 && hs->spice_level > 75) {
        slot.primary = DEG_bVI7; slot.secondary = DEG_V7; slot.has_split = 1;
    }
    if (hs->current_bar == 11 && hs->spice_level > 40) {
        slot.primary = (hs->spice_level > 75) ? DEG_bVI7 : DEG_I7;
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
                hs->ending_armed = 1;
                break;
            case CUE_DRUM_FILL:
                hs->fill_pending = 1;
                break;
            default:
                break;
        }
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
        if (hs->ending_armed) {
            hs->is_playing = 0; // All outro cadences conclude and stop playback
            hs->ending_armed = 0;
        }
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
