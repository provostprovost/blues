#include "blues_arranger.h"

static const GrooveTemplate GROOVE_TEMPLATES[5] = {
    { GROOVE_SLOW_12_8, 67,
      {127,0,0,90, 100,0,0,90, 110,0,0,90, 100,0,0,90},
      {100,0,0,0,  0,0,80,0,   100,0,0,0,  0,0,80,0},
      {110,70,70,110, 70,70,110,70, 70,110,70,70, 110,70,70,110} },
    { GROOVE_TEXAS_SHUFFLE, 67,
      {127,0,90,0, 110,0,90,0, 120,0,90,0, 110,0,90,0},
      {100,0,0,0,  90,0,0,0,   100,0,0,0,  90,0,0,0},
      {127,0,100,0, 127,0,100,0, 127,0,100,0, 127,0,100,0} },
    { GROOVE_CHICAGO_SWING, 60,
      {127,0,0,0,  100,0,0,0,  110,0,0,0,  100,0,0,0},
      {110,0,0,80, 0,0,100,0,  110,0,0,80, 0,0,100,0},
      {127,0,80,0, 127,0,80,0,  127,0,80,0, 127,0,80,0} },
    { GROOVE_STRAIGHT_ROCK, 50,
      {127,0,100,0, 110,0,100,0, 127,0,100,0, 110,0,100,0},
      {127,0,0,0,  0,0,0,0,    127,0,0,0,  0,0,0,0},
      {100,100,100,100, 100,100,100,100, 100,100,100,100, 100,100,100,100} },
    { GROOVE_SECOND_LINE, 55,
      {127,0,90,0, 0,90,110,0, 127,0,90,0, 0,90,110,0},
      {100,0,0,90, 0,100,0,0,  100,0,0,90, 0,100,0,0},
      {127,90,100,90, 127,90,100,90, 127,90,100,90, 127,90,100,90} }
};

static uint16_t g_hmnz_lfsr = 0x5EEDu;
static inline int8_t get_hmnz_jitter(uint8_t spice) {
    if (spice == 0) return 0;
    g_hmnz_lfsr = (g_hmnz_lfsr >> 1) ^ (-(g_hmnz_lfsr & 1u) & 0xB400u);
    int8_t raw = (int8_t)(g_hmnz_lfsr & 0x0F) - 8;
    return (int8_t)((raw * (int)spice) / 100);
}

uint8_t bass_compute_note(ChordContext current, ChordContext next, uint8_t beat, uint8_t sub_idx, uint8_t spice) {
    uint8_t r = current.root_note % 12;
    uint8_t curr_root = (uint8_t)(40 + ((r + 8) % 12)); // Map to E2..Eb3 (40..51)
    while (curr_root < 40) curr_root += 12;
    while (curr_root > 51) curr_root -= 12;

    uint8_t nr = next.root_note % 12;
    uint8_t next_target = (uint8_t)(40 + ((nr + 8) % 12));
    while (next_target < 40) next_target += 12;
    while (next_target > 51) next_target -= 12;

    uint8_t note = curr_root;

    if (sub_idx == 2 && spice > 75) {
        note = curr_root + 12;
    } else if (sub_idx == 2) {
        note = curr_root + 7;
    } else if (sub_idx == 1 && spice > 75) {
        note = curr_root + 2;
    } else if (sub_idx == 3 && spice > 75) {
        note = (next_target > 40) ? (next_target - 1) : (next_target + 1);
    } else {
        switch (beat) {
            case 0:
                note = curr_root;
                break;
            case 1:
                if (current.quality == CHORD_MIN7 || current.quality == CHORD_DIM7) {
                    note = curr_root + 3;
                } else {
                    note = curr_root + 4;
                }
                break;
            case 2:
                if (current.quality == CHORD_DIM7) {
                    note = curr_root + 6;
                } else if (spice > 50) {
                    note = curr_root + 9;
                } else {
                    note = curr_root + 7;
                }
                break;
            case 3:
            default: {
                if (spice < 30) {
                    note = (next_target >= 47) ? (next_target - 7) : (next_target + 5);
                } else if (next_target >= curr_root) {
                    note = (next_target > 40) ? (next_target - 1) : (next_target + 1);
                } else {
                    note = (next_target < 61) ? (next_target + 1) : (next_target - 1);
                }
                break;
            }
        }
    }

    while (note < 40) note += 12;
    while (note > 62) note -= 12;
    return note;
}

Voicing voice_lead_select(Voicing prev_voicing, ScaleDegree degree, uint8_t root_semitone, uint8_t spice) {
    Voicing candidates[8];
    ScaleDegreeInfo info = harmony_get_degree_info(degree);
    uint8_t root = (root_semitone + info.semitone_offset) % 12;
    uint8_t base_root = 48 + root; // C3 + root
    uint8_t third = (info.quality == CHORD_MIN7 || info.quality == CHORD_DIM7) ? 3 : 4;
    uint8_t fifth = (info.quality == CHORD_DIM7) ? 6 : 7;
    uint8_t seventh = (info.quality == CHORD_DIM7) ? 9 : 10;
    uint8_t ninth = 14;

    // Type 0: Root Position (1 - 3 - 5 - b7)
    candidates[0].count = 4;
    candidates[0].notes[0] = base_root;
    candidates[0].notes[1] = base_root + third;
    candidates[0].notes[2] = base_root + fifth;
    candidates[0].notes[3] = base_root + seventh;

    // Type 1: Inversion (5 - b7 - 1 - 3)
    candidates[1].count = 4;
    candidates[1].notes[0] = (uint8_t)(base_root + fifth - 12);
    candidates[1].notes[1] = (uint8_t)(base_root + seventh - 12);
    candidates[1].notes[2] = base_root;
    candidates[1].notes[3] = base_root + third;

    // Type 2: 9th Color Voicing (3 - b7 - 9 - 1)
    candidates[2].count = 4;
    candidates[2].notes[0] = base_root + third;
    candidates[2].notes[1] = base_root + seventh;
    candidates[2].notes[2] = base_root + ninth;
    candidates[2].notes[3] = base_root + 12;

    // Type 3: Lower Inversion with 9th (b7 - 3 - 1 - 9)
    candidates[3].count = 4;
    candidates[3].notes[0] = (uint8_t)(base_root + seventh - 12);
    candidates[3].notes[1] = base_root + third;
    candidates[3].notes[2] = base_root;
    candidates[3].notes[3] = base_root + ninth;

    // Octave Variations for candidates 0..3 -> 4..7
    for (int i = 0; i < 4; i++) {
        candidates[i + 4] = candidates[i];
        for (int n = 0; n < 4; n++) {
            if (candidates[i].notes[n] <= 64) candidates[i + 4].notes[n] += 12;
            else candidates[i + 4].notes[n] -= 12;
        }
    }

    // Clamp each candidate strictly to C3..E5 (48..76) and sort notes
    uint8_t target_count = 4;
    if (spice <= 25) target_count = 2;
    else if (spice <= 50) target_count = 3;

    for (int i = 0; i < 8; i++) {
        for (int n = 0; n < 4; n++) {
            while (candidates[i].notes[n] < 48) candidates[i].notes[n] += 12;
            while (candidates[i].notes[n] > 76) candidates[i].notes[n] -= 12;
        }
        for (int a = 0; a < 3; a++) {
            for (int b = a + 1; b < 4; b++) {
                if (candidates[i].notes[a] > candidates[i].notes[b]) {
                    uint8_t tmp = candidates[i].notes[a];
                    candidates[i].notes[a] = candidates[i].notes[b];
                    candidates[i].notes[b] = tmp;
                }
            }
        }
        candidates[i].count = target_count;
    }

    if (prev_voicing.count == 0) return candidates[0];

    int32_t min_dist = 0x7FFF;
    uint8_t best_idx = 0;
    for (uint8_t i = 0; i < 8; i++) {
        int32_t dist = 0;
        uint8_t check_count = (candidates[i].count < prev_voicing.count) ? candidates[i].count : prev_voicing.count;
        for (uint8_t n = 0; n < check_count; n++) {
            int diff = (int)candidates[i].notes[n] - (int)prev_voicing.notes[n];
            dist += (diff < 0) ? -diff : diff;
        }
        if (dist < min_dist) {
            min_dist = dist;
            best_idx = i;
        }
    }
    return candidates[best_idx];
}

void arranger_generate_frame(HarmonyState *hs, const PresetConfig *preset, PerformanceFrame *frame, Voicing *last_voicing) {
    frame->bass_trigger = 0;
    frame->bass_vel = 0;
    frame->comp_trigger = 0;
    frame->comp_vel = 0;
    frame->drum_mask = 0;
    for (int d = 0; d < 4; d++) frame->drum_vel[d] = 0;

    if (!hs->is_playing) return;

    if (hs->count_in_bars_remaining > 0) {
        if (hs->current_subdiv == 0) {
            frame->drum_mask = 0x08; // Rimshot click
            frame->drum_vel[3] = 90;
        }
        return;
    }

    if (hs->stop_time_counter > 0) {
        if (hs->current_beat > 0) return;
    }

    const GrooveTemplate *gt = &GROOVE_TEMPLATES[preset->groove_style % 5];
    uint8_t sub_slot = 0xFF;

    if (gt->style == GROOVE_SLOW_12_8) {
        if (hs->current_subdiv == 0) sub_slot = 0;
        else if (hs->current_subdiv == 8) sub_slot = 1;
        else if (hs->current_subdiv == 16) sub_slot = 2;
    } else {
        uint8_t offbeat_subdiv = (uint8_t)((24 * preset->swing) / 100);
        if (hs->current_subdiv == 0) sub_slot = 0;
        else if (hs->current_subdiv == 6 && preset->swing < 60) sub_slot = 1;
        else if (hs->current_subdiv == offbeat_subdiv) sub_slot = 2;
        else if (hs->current_subdiv == 18 && preset->swing < 60) sub_slot = 3;
    }

    if (sub_slot != 0xFF) {
        uint8_t step16 = (uint8_t)((hs->current_beat * 4) + sub_slot);
        BarSlot curr_slot = harmony_get_current_bar_slot(hs);
        ScaleDegree active_degree = (hs->current_beat >= 2 && curr_slot.has_split) ? curr_slot.secondary : curr_slot.primary;
        int8_t j_vel = get_hmnz_jitter(hs->spice_level);

        // Bass engine trigger
        if (gt->bass_mask[step16 % 16] > 0 || (hs->spice_level > 75 && (sub_slot == 1 || sub_slot == 3))) {
            frame->bass_trigger = 1;
            int base_v = (gt->bass_mask[step16 % 16] > 0) ? gt->bass_mask[step16 % 16] : 85;
            int bv = base_v + j_vel;
            frame->bass_vel = (uint8_t)((bv < 1) ? 1 : (bv > 127) ? 127 : bv);

            BarSlot next_slot = harmony_get_next_bar_slot(hs);
            ScaleDegreeInfo curr_info = harmony_get_degree_info(active_degree);
            ScaleDegreeInfo next_info = harmony_get_degree_info(next_slot.primary);
            ChordContext c_curr = { (uint8_t)((hs->root_semitone + curr_info.semitone_offset) % 12), curr_info.quality };
            ChordContext c_next = { (uint8_t)((hs->root_semitone + next_info.semitone_offset) % 12), next_info.quality };
            frame->bass_note = bass_compute_note(c_curr, c_next, hs->current_beat, sub_slot, hs->spice_level);
        }

        // Comping engine trigger
        if (gt->comp_mask[step16 % 16] > 0 || hs->manual_stab_pending) {
            frame->comp_trigger = 1;
            int base_cv = gt->comp_mask[step16 % 16];
            if (hs->manual_stab_pending) base_cv = 120;
            int cv = base_cv + j_vel;
            frame->comp_vel = (uint8_t)((cv < 1) ? 1 : (cv > 127) ? 127 : cv);
            frame->comp_voicing = voice_lead_select(*last_voicing, active_degree, hs->root_semitone, hs->spice_level);
            *last_voicing = frame->comp_voicing;
            hs->manual_stab_pending = 0;
        }

        // Drum channels: Hi-hat / Ride
        if (gt->drum_hat_mask[step16 % 16] > 0) {
            frame->drum_mask |= 0x04;
            int dv = gt->drum_hat_mask[step16 % 16] + j_vel;
            frame->drum_vel[2] = (uint8_t)((dv < 1) ? 1 : (dv > 127) ? 127 : dv);
        }

        // Kick Drum: Beat 0 downbeat
        if (hs->current_beat == 0 && hs->current_subdiv == 0) {
            frame->drum_mask |= 0x01;
            int dv0 = 110 + j_vel;
            frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
        }
        // Syncopated kick on beat 3 offbeat at high Spice
        if (hs->spice_level > 75 && sub_slot == 2 && hs->current_beat == 3) {
            frame->drum_mask |= 0x01;
            int dv0 = 95 + j_vel;
            frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
        }

        // Snare: Backbeat on Beats 1 and 3
        if ((hs->current_beat == 1 || hs->current_beat == 3) && hs->current_subdiv == 0) {
            frame->drum_mask |= 0x02;
            int dv1 = 100 + j_vel;
            frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
        }
        // Snare ghost note on beat 2 offbeat at Spice > 25
        if (hs->spice_level > 25 && sub_slot == 2 && hs->current_beat == 2) {
            frame->drum_mask |= 0x02;
            int dv1 = 45 + j_vel;
            frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
        }

        // Dynamic fill on Bar 11 beat 3 or manual fill trigger
        if ((hs->current_bar == 11 && hs->spice_level > 50) || hs->fill_pending) {
            if (hs->current_beat == 3) {
                frame->drum_mask |= 0x08; // Rimshot / fill
                int dv3 = 95 + j_vel;
                frame->drum_vel[3] = (uint8_t)((dv3 < 1) ? 1 : (dv3 > 127) ? 127 : dv3);
            }
            if (hs->current_beat == 3 && hs->current_subdiv == 12) hs->fill_pending = 0;
        }
    }
}
