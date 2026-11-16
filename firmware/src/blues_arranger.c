#include "blues_arranger.h"

static const GrooveTemplate GROOVE_TEMPLATES[8] = {
    { GROOVE_SLOW_12_8, 67,
      {127,0,0,90, 100,0,0,90, 110,0,0,90, 100,0,0,90},
      {100,0,0,0,  0,0,80,0,   100,0,0,0,  0,0,80,0},
      {110,70,70,110, 70,70,110,70, 70,110,70,70, 110,70,70,110} },
    { GROOVE_TEXAS_SHUFFLE, 67,
      {127,0,90,0, 110,0,90,0, 120,0,90,0, 110,0,90,0},
      {100,0,0,0,  90,0,0,0,   100,0,0,0,  90,0,0,0},
      {127,0,100,0, 127,0,100,0, 127,0,100,0, 127,0,100,0} },
    { GROOVE_CHICAGO_SWING, 55,
      {127,0,0,0,  100,0,0,0,  110,0,0,0,  100,0,0,0},
      {110,0,0,80, 0,0,100,0,  110,0,0,80, 0,0,100,0},
      {127,0,80,0, 127,0,80,0,  127,0,80,0, 127,0,80,0} },
    { GROOVE_STRAIGHT_ROCK, 0,
      {127,0,100,0, 110,0,100,0, 127,0,100,0, 110,0,100,0},
      {127,0,0,0,  0,0,0,0,    127,0,0,0,  0,0,0,0},
      {100,100,100,100, 100,100,100,100, 100,100,100,100, 100,100,100,100} },
    { GROOVE_SECOND_LINE, 25,
      {127,0,90,0, 0,90,110,0, 127,0,90,0, 0,90,110,0},
      {100,0,0,90, 0,100,0,0,  100,0,0,90, 0,100,0,0},
      {127,90,100,90, 127,90,100,90, 127,90,100,90, 127,90,100,90} },
    { GROOVE_MEMPHIS_SOUL, 15,
      {127,0,90,0, 0,90,110,0, 127,0,90,0, 0,90,110,0},
      {0,0,100,0,  110,0,0,90, 0,0,100,0,  110,0,0,90},
      {120,80,100,80, 120,80,100,80, 120,80,100,80, 120,80,100,80} },
    { GROOVE_RHUMBA_BLUES, 0,
      {127,0,0,100, 0,0,110,0, 127,0,0,100, 0,0,110,0},
      {0,0,90,0,   110,0,0,0,  0,0,90,0,   110,0,0,0},
      {110,70,70,100, 70,70,110,70, 110,70,70,100, 70,70,110,70} },
    { GROOVE_BOOGIE_WOOGIE, 65,
      {127,0,110,0, 120,0,110,0, 127,0,110,0, 120,0,110,0},
      {0,0,0,0,    110,0,0,0,  0,0,0,0,    110,0,0,0},
      {127,0,90,0, 127,0,90,0, 127,0,90,0, 127,0,90,0} }
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
    uint8_t curr_root = (uint8_t)(40 + ((r + 8) % 12)); // Map to E2..Eb3 (40..51, 82..155 Hz for pocket speaker projection)
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
    Voicing candidates[4];
    ScaleDegreeInfo info = harmony_get_degree_info(degree);
    uint8_t root = (root_semitone + info.semitone_offset) % 12;
    uint8_t third = (info.quality == CHORD_MIN7 || info.quality == CHORD_DIM7) ? 3 : 4;
    uint8_t fifth = (info.quality == CHORD_DIM7) ? 6 : 7;
    uint8_t seventh = (info.quality == CHORD_DIM7) ? 9 : 10;
    uint8_t ninth = 14;

    // Pitch anchor: ensure guide tones sit in sparkling high comping register (C5..E6, 520..1320 Hz)
    uint8_t t_note = 72 + ((root + third) % 12);
    while (t_note < 72) t_note += 12; // C5..B5 (72..83)
    while (t_note > 83) t_note -= 12;

    uint8_t s_note = 72 + ((root + seventh) % 12);
    while (s_note < 72) s_note += 12; // C5..B5 (72..83)
    while (s_note > 83) s_note -= 12;

    uint8_t n_note = 72 + ((root + ninth) % 12);
    while (n_note < 74) n_note += 12; // D5..C#6 (74..85)
    while (n_note > 85) n_note -= 12;

    uint8_t f_note = 72 + ((root + fifth) % 12);
    while (f_note < 72) f_note += 12; // C5..B5 (72..83)
    while (f_note > 83) f_note -= 12;

    uint8_t th_note = 72 + ((root + 21) % 12); // 13th
    while (th_note < 76) th_note += 12; // E5..Eb6 (76..87)
    while (th_note > 87) th_note -= 12;

    // Candidate 0: Authentic Rootless Shell / 9th (3rd + 7th + 5th + 9th)
    candidates[0].notes[0] = t_note;
    candidates[0].notes[1] = s_note;
    candidates[0].notes[2] = f_note;
    candidates[0].notes[3] = n_note;
    candidates[0].count = (spice <= 30) ? 2 : (spice <= 60) ? 3 : 4;

    // Candidate 1: Bill Evans Form A (3rd + 7th + 9th + 13th)
    candidates[1].notes[0] = t_note;
    candidates[1].notes[1] = s_note;
    candidates[1].notes[2] = n_note;
    candidates[1].notes[3] = th_note;
    candidates[1].count = (spice <= 30) ? 2 : (spice <= 60) ? 3 : 4;

    // Candidate 2: Bill Evans Form B Inversion (7th on bottom + 3rd + 13th + 9th)
    candidates[2].notes[0] = (uint8_t)(s_note > 78 ? s_note - 12 : s_note);
    candidates[2].notes[1] = t_note;
    candidates[2].notes[2] = th_note;
    candidates[2].notes[3] = n_note;
    candidates[2].count = (spice <= 30) ? 2 : (spice <= 60) ? 3 : 4;

    // Candidate 3: Open 4ths / Organ Comp (3rd + 13th + 7th + 9th)
    candidates[3].notes[0] = t_note;
    candidates[3].notes[1] = th_note;
    candidates[3].notes[2] = s_note;
    candidates[3].notes[3] = n_note;
    candidates[3].count = (spice <= 30) ? 2 : (spice <= 60) ? 3 : 4;

    // Sort notes ascending within each candidate to guarantee clean register ordering
    for (int i = 0; i < 4; i++) {
        for (int a = 0; a < 3; a++) {
            for (int b = a + 1; b < 4; b++) {
                if (candidates[i].notes[a] > candidates[i].notes[b]) {
                    uint8_t tmp = candidates[i].notes[a];
                    candidates[i].notes[a] = candidates[i].notes[b];
                    candidates[i].notes[b] = tmp;
                }
            }
        }
    }

    if (prev_voicing.count == 0) return candidates[0];

    // Pick candidate with minimum voice-leading distance from previous chord
    int32_t min_dist = 0x7FFF;
    uint8_t best_idx = 0;
    for (uint8_t i = 0; i < 4; i++) {
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

    const GrooveTemplate *gt = &GROOVE_TEMPLATES[preset->groove_style % 8];
    uint8_t sub_slot = 0xFF;

    if (gt->style == GROOVE_SLOW_12_8) {
        if (hs->current_subdiv == 0) sub_slot = 0;
        else if (hs->current_subdiv == 8) sub_slot = 1;
        else if (hs->current_subdiv == 16) sub_slot = 2;
    } else {
        uint8_t offbeat_subdiv = 12 + (uint8_t)((preset->swing * 6) / 100);
        if (hs->current_subdiv == 0) sub_slot = 0;
        else if (hs->current_subdiv == 6 && preset->swing < 30) sub_slot = 1;
        else if (hs->current_subdiv == offbeat_subdiv) sub_slot = 2;
        else if (hs->current_subdiv == 18 && preset->swing < 30) sub_slot = 3;
    }

    if (sub_slot != 0xFF) {
        uint8_t step16 = (uint8_t)((hs->current_beat * 4) + sub_slot);
        BarSlot curr_slot = harmony_get_current_bar_slot(hs);
        ScaleDegree active_degree = (hs->current_beat >= 2 && curr_slot.has_split) ? curr_slot.secondary : curr_slot.primary;
        int8_t j_vel = get_hmnz_jitter(hs->spice_level);
        uint8_t full = hs->fullness;

        // Bass engine trigger (shaped by Fullness: sparse half-notes <35, template 35..70, driving >70)
        uint8_t bass_hit = (gt->bass_mask[step16 % 16] > 0);
        if (full < 35) {
            bass_hit = ((hs->current_beat == 0 || hs->current_beat == 2) && sub_slot == 0);
        } else if (full > 70 && ((sub_slot == 2 && (hs->current_beat == 1 || hs->current_beat == 3)) ||
                                 (full > 85 && (sub_slot == 1 || sub_slot == 3)))) {
            bass_hit = 1;
        }
        if (bass_hit) {
            frame->bass_trigger = 1;
            int base_v = (gt->bass_mask[step16 % 16] > 0) ? gt->bass_mask[step16 % 16] : 88;
            int bv = base_v + j_vel + ((full > 75) ? 8 : (full < 35) ? -10 : 0);
            frame->bass_vel = (uint8_t)((bv < 1) ? 1 : (bv > 127) ? 127 : bv);

            BarSlot next_slot = harmony_get_next_bar_slot(hs);
            ScaleDegreeInfo curr_info = harmony_get_degree_info(active_degree);
            ScaleDegreeInfo next_info = harmony_get_degree_info(next_slot.primary);
            ChordContext c_curr = { (uint8_t)((hs->root_semitone + curr_info.semitone_offset) % 12), curr_info.quality };
            ChordContext c_next = { (uint8_t)((hs->root_semitone + next_info.semitone_offset) % 12), next_info.quality };
            frame->bass_note = bass_compute_note(c_curr, c_next, hs->current_beat, sub_slot, hs->spice_level);
        }

        // Comping engine trigger (shaped by Fullness: sparse backbeat <35, template 35..70, push stabs >70)
        uint8_t comp_hit = (gt->comp_mask[step16 % 16] > 0);
        if (full < 35) {
            comp_hit = ((hs->current_beat == 1 || hs->current_beat == 3) && sub_slot == 0);
        } else if (full > 75 && sub_slot == 2 && (hs->current_beat == 1 || hs->current_beat == 3)) {
            comp_hit = 1;
        }
        if (comp_hit || hs->manual_stab_pending) {
            frame->comp_trigger = 1;
            int base_cv = (gt->comp_mask[step16 % 16] > 0) ? gt->comp_mask[step16 % 16] : 85;
            if (hs->manual_stab_pending) base_cv = 120;
            int cv = base_cv + j_vel + ((full > 75) ? 12 : (full < 35) ? -18 : 0);
            frame->comp_vel = (uint8_t)((cv < 1) ? 1 : (cv > 127) ? 127 : cv);
            frame->comp_voicing = voice_lead_select(*last_voicing, active_degree, hs->root_semitone, hs->spice_level);
            *last_voicing = frame->comp_voicing;
            hs->manual_stab_pending = 0;
        }

        // Drum channels: Hi-hat / Ride
        if (gt->drum_hat_mask[step16 % 16] > 0) {
            frame->drum_mask |= 0x04;
            int dv = gt->drum_hat_mask[step16 % 16] + j_vel + ((full < 35) ? -20 : (full > 75) ? 8 : 0);
            frame->drum_vel[2] = (uint8_t)((dv < 1) ? 1 : (dv > 127) ? 127 : dv);
        }

        // Kick Drum: Beat 0 downbeat (+ Beat 2 in rocking fullness)
        if (hs->current_beat == 0 && hs->current_subdiv == 0) {
            frame->drum_mask |= 0x01;
            int dv0 = 110 + j_vel + ((full > 75) ? 10 : (full < 35) ? -15 : 0);
            frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
        }
        if (full > 70 && sub_slot == 2 && hs->current_beat == 3) {
            frame->drum_mask |= 0x01;
            int dv0 = 95 + j_vel;
            frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
        }

        // Snare / Rimshot: Backbeat on Beats 1 and 3
        if ((hs->current_beat == 1 || hs->current_beat == 3) && hs->current_subdiv == 0) {
            if (full < 20) {
                frame->drum_mask |= 0x08; // Ultra-sparse: cross-stick rimshot instead of snare
                int dv3 = 80 + j_vel;
                frame->drum_vel[3] = (uint8_t)((dv3 < 1) ? 1 : (dv3 > 127) ? 127 : dv3);
            } else {
                frame->drum_mask |= 0x02;
                int dv1 = 100 + j_vel + ((full > 75) ? 10 : (full < 35) ? -18 : 0);
                frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
            }
        }
        // Snare ghost note on beat 2 offbeat at Fullness >= 40
        if (full >= 40 && sub_slot == 2 && hs->current_beat == 2) {
            frame->drum_mask |= 0x02;
            int dv1 = 45 + j_vel + ((full > 75) ? 12 : 0);
            frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
        }

        // Automatic phrase-end drum fills on Bar 4, Bar 8, and Bar 12 (or manual fill)
        uint8_t is_phrase_end = (hs->current_bar == 3 || hs->current_bar == 7 || hs->current_bar == 11);
        if ((is_phrase_end && full >= 25) || hs->fill_pending) {
            if (hs->current_beat == 3) {
                uint8_t is_turnaround_bar = (hs->current_bar == 11 || hs->fill_pending);
                if (sub_slot == 0) {
                    frame->drum_mask |= 0x02;
                    int dv1 = (is_turnaround_bar ? 105 : 88) + j_vel;
                    frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
                } else if (sub_slot == 2) {
                    frame->drum_mask |= 0x03; // Kick + Snare pickup into Beat 1
                    int dv0 = 85 + j_vel;
                    int dv1 = (is_turnaround_bar ? 112 : 95) + j_vel;
                    frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
                    frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
                    if (is_turnaround_bar) {
                        frame->drum_mask |= 0x08;
                        frame->drum_vel[3] = 85;
                    }
                    hs->fill_pending = 0;
                }
            }
        }
    }
}
