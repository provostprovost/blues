#include "blues_arranger.h"
#include "blues_harmony.h"

static const GrooveTemplate GROOVE_TEMPLATES[NUM_GROOVES] = {
    { GROOVE_SLOW_12_8, 67,
      {127,0,0,90, 100,0,0,90, 110,0,0,90, 100,0,0,90},
      {100,0,0,0,  0,0,80,0,   100,0,0,0,  0,0,80,0},
      {110,70,70,110, 70,70,110,70, 70,110,70,70, 110,70,70,110} },
    { GROOVE_TEXAS_SHUFFLE, 67,
      {127,0,90,0, 110,0,90,0, 120,0,90,0, 110,0,90,0},
      {100,0,0,0,  90,0,0,0,   100,0,0,0,  90,0,0,0},
      {127,0,100,0, 127,0,100,0, 127,0,100,0, 127,0,100,0} },
    { GROOVE_CHICAGO_SWING, 67,
      {127,0,0,0,  100,0,0,0,  110,0,0,0,  100,0,0,0},
      {110,0,0,80, 0,0,100,0,  110,0,0,80, 0,0,100,0},
      {127,0,80,0, 127,0,80,0,  127,0,80,0, 127,0,80,0} },
    { GROOVE_STRAIGHT_ROCK, 0,
      {127,0,100,0, 110,0,100,0, 127,0,100,0, 110,0,100,0},
      {127,0,0,0,  0,0,0,0,    127,0,0,0,  0,0,0,0},
      {100,100,100,100, 100,100,100,100, 100,100,100,100, 100,100,100,100} },
    { GROOVE_SECOND_LINE, 0,
      {127,0,90,0, 0,90,110,0, 127,0,90,0, 0,90,110,0},
      {100,0,0,90, 0,100,0,0,  100,0,0,90, 0,100,0,0},
      {127,90,100,90, 127,90,100,90, 127,90,100,90, 127,90,100,90} },
    { GROOVE_MEMPHIS_SOUL, 0,
      {127,0,90,0, 0,90,110,0, 127,0,90,0, 0,90,110,0},
      {0,0,100,0,  110,0,0,90, 0,0,100,0,  110,0,0,90},
      {120,80,100,80, 120,80,100,80, 120,80,100,80, 120,80,100,80} },
    { GROOVE_RHUMBA_BLUES, 0,
      {127,0,0,100, 0,0,110,0, 127,0,0,100, 0,0,110,0},
      {0,0,90,0,   110,0,0,0,  0,0,90,0,   110,0,0,0},
      {110,70,70,100, 70,70,110,70, 110,70,70,100, 70,70,110,70} },
    { GROOVE_BOOGIE_WOOGIE, 67,
      {127,0,110,0, 120,0,110,0, 127,0,110,0, 120,0,110,0},
      {0,0,0,0,    110,0,0,0,  0,0,0,0,    110,0,0,0},
      {127,0,90,0, 127,0,90,0, 127,0,90,0, 127,0,90,0} },
    { GROOVE_PURDIE_SHUFFLE, 67, // Half-time shuffle: Beat 3 crack, syncopated kick, ghost snares
      {127,0,65,0, 0,0,95,0,   120,0,60,0, 0,0,85,0},
      {115,0,0,0,  0,0,90,0,   0,0,95,0,   0,0,0,0},
      {127,0,105,0, 115,0,105,0, 127,0,105,0, 115,0,105,0} },
    { GROOVE_TRAIN_BEAT, 0, // Locomotive chug: steady 4-on-floor kick + accented 16th snare train
      {110,60,127,60, 110,60,127,60, 110,60,127,60, 110,60,127,60},
      {110,0,0,0,  100,0,0,0,  110,0,0,0,  100,0,0,0},
      {95,0,80,0,  95,0,80,0,  95,0,80,0,  95,0,80,0} },
    { GROOVE_BO_DIDDLEY, 0, // Classic 3-2 clave / Bo Diddley accent pattern
      {127,0,0,110, 0,0,115,0, 0,0,105,0, 115,0,0,0},
      {120,0,0,0,  0,0,95,0,   110,0,0,0, 0,0,90,0},
      {105,0,90,0, 105,0,90,0, 105,0,90,0, 105,0,90,0} },
    { GROOVE_JAZZ_SHUFFLE, 67, // Feathered kick + crisp 2&4 + classic ride skip
      {95,0,0,0,   110,0,0,0,  95,0,0,0,   110,0,0,0},
      {90,0,0,0,   0,0,0,0,    90,0,0,0,   0,0,0,0},
      {120,0,0,0,  127,0,105,0, 120,0,0,0, 127,0,105,0} },
    { GROOVE_GOSPEL_SWING, 67, // Big church 1 & 3 kick + 2 & 4 snare + 4& push
      {127,0,0,0,  120,0,0,0,  127,0,0,0,  120,0,90,0},
      {120,0,0,0,  0,0,85,0,   120,0,0,0,  0,0,95,0},
      {115,0,95,0, 115,0,95,0, 115,0,95,0, 115,0,95,0} },
    { GROOVE_DELTA_STOMP, 67, // Heavy 4-on-floor foot stomp + backbeat clap/snare
      {110,0,0,0,  127,0,0,0,  110,0,0,0,  127,0,0,0},
      {127,0,0,0,  120,0,0,0,  127,0,0,0,  120,0,0,0},
      {90,0,0,0,   100,0,0,0,  90,0,0,0,   100,0,0,0} },
    { GROOVE_POCKET_FUNK, 0, // 70s Pocket Funk: tight straight 8th/16th hats, kick on 1 & 2& & 3, snare on 2 & 4
      {127,0,0,0,  0,0,115,0,  120,0,95,0,  0,0,105,0},
      {110,0,0,0,  0,0,95,0,   0,0,0,0,     100,0,0,0},
      {115,75,100,75, 115,75,100,75, 115,75,100,75, 115,75,100,75} },
    { GROOVE_NOLA_FUNK, 0, // Meters-style New Orleans syncopated second-line funk
      {127,0,90,0, 0,0,115,0,  118,0,0,0,   0,0,110,0},
      {105,0,0,0,  0,0,95,0,   0,0,90,0,    0,0,0,0},
      {120,0,100,0, 120,0,100,0, 120,0,100,0, 120,0,100,0} },
    { GROOVE_BLUES_FUNK, 67, // Greasy triplet-swung blues-funk pocket
      {127,0,0,0,  0,0,112,0,  120,0,95,0,  0,0,105,0},
      {0,0,95,0,   110,0,0,0,  0,0,95,0,    110,0,0,0},
      {122,0,98,0, 122,0,98,0, 122,0,98,0,  122,0,98,0} },
    { GROOVE_SLOW_NEO_FUNK, 0, // Laid-back straight neo-funk / soul pocket
      {127,0,0,0,  0,0,105,0,  118,0,0,0,   0,0,98,0},
      {95,0,0,0,   0,0,90,0,   0,0,0,0,     92,0,0,0},
      {112,0,90,0, 112,0,90,0, 112,0,90,0,  112,0,90,0} },
    { GROOVE_COUNTRY_TWANG, 67, // Country shuffle boom-chick with alternating root-5th bass
      {127,0,0,0,  0,0,0,0,    122,0,0,0,   0,0,95,0},
      {0,0,0,0,    110,0,0,0,  0,0,0,0,     110,0,0,0},
      {115,0,95,0, 115,0,95,0, 115,0,95,0,  115,0,95,0} },
    { GROOVE_SWAMP_ROCK, 0, // Lucinda "Righteously" Swamp Rock: driving 16th-note hats + double-time swamp pocket (~104 BPM)
      {127,0,98,0,  0,0,108,0, 122,0,98,0,  0,0,108,0},
      {0,0,122,0,   0,0,0,0,   0,0,122,0,   0,0,90,0},
      {115,82,102,82, 115,82,102,82, 115,82,102,82, 115,82,102,82} }
};

static uint16_t g_hmnz_lfsr = 0x5EEDu;
static inline int8_t get_hmnz_jitter(uint8_t variation) {
    if (variation == 0) return 0;
    g_hmnz_lfsr = (g_hmnz_lfsr >> 1) ^ (-(g_hmnz_lfsr & 1u) & 0xB400u);
    int8_t raw = (int8_t)(g_hmnz_lfsr & 0x0F) - 8;
    return (int8_t)((raw * (int)variation) / 100);
}

uint8_t bass_compute_note(ChordContext current, ChordContext next, uint8_t beat, uint8_t sub_idx, uint8_t variation) {
    uint8_t r = current.root_note % 12;
    uint8_t curr_root = (uint8_t)(40 + ((r + 8) % 12)); // Map to E2..Eb3 (40..51, 82..155 Hz for pocket speaker projection)
    while (curr_root < 40) curr_root += 12;
    while (curr_root > 51) curr_root -= 12;

    uint8_t nr = next.root_note % 12;
    uint8_t next_target = (uint8_t)(40 + ((nr + 8) % 12));
    while (next_target < 40) next_target += 12;
    while (next_target > 51) next_target -= 12;

    uint8_t note = curr_root;

    if (sub_idx == 2 && variation > 75) {
        note = curr_root + 10; // b7
    } else if (sub_idx == 2) {
        note = curr_root + 7;  // 5th
    } else if (sub_idx == 1 && variation > 75) {
        note = curr_root + ((current.quality == CHORD_MIN7 || current.quality == CHORD_DIM7) ? 3 : 4);
    } else if (sub_idx == 3 && variation > 75 && next_target != curr_root) {
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
                } else {
                    note = curr_root + 7; // Solid 5th on Beat 3
                }
                break;
            case 3:
            default: {
                if (current.quality == CHORD_DIM7) {
                    note = curr_root + 9;
                } else if (current.quality == CHORD_MIN7) {
                    note = curr_root + 10; // b7 on minor blues
                } else if (variation > 55) {
                    note = curr_root + 9;  // Classic 6th in major blues box (1 - 3 - 5 - 6)
                } else {
                    note = curr_root + 10; // b7
                }
                break;
            }
        }
    }

    while (note < 40) note += 12;
    while (note > 62) note -= 12;
    return note;
}

Voicing voice_lead_select(Voicing prev_voicing, ScaleDegree degree, uint8_t root_semitone, uint8_t variation) {
    Voicing candidates[4];
    ScaleDegreeInfo info = harmony_get_degree_info(degree);
    uint8_t root = (root_semitone + info.semitone_offset) % 12;
    uint8_t third = (info.quality == CHORD_MIN7 || info.quality == CHORD_DIM7) ? 3 : 4;
    uint8_t fifth = (info.quality == CHORD_DIM7) ? 6 : 7;
    uint8_t seventh = (info.quality == CHORD_DIM7) ? 9 : 10;
    uint8_t ninth = 14;

    // Pitch anchor: MUST add to 60 (C4, multiple of 12!) so pitch classes are never transposed, then wrap to F4..Ab5 (65..80)
    uint8_t r_note = 60 + (root % 12);
    while (r_note < 65) r_note += 12; // F4..E5 (65..76)
    while (r_note > 76) r_note -= 12;

    uint8_t t_note = 60 + ((root + third) % 12);
    while (t_note < 65) t_note += 12; // F4..E5 (65..76)
    while (t_note > 76) t_note -= 12;

    uint8_t s_note = 60 + ((root + seventh) % 12);
    while (s_note < 65) s_note += 12; // F4..E5 (65..76)
    while (s_note > 76) s_note -= 12;

    uint8_t f_note = 60 + ((root + fifth) % 12);
    while (f_note < 65) f_note += 12; // F4..E5 (65..76)
    while (f_note > 76) f_note -= 12;

    uint8_t n_note = 60 + ((root + ninth) % 12);
    while (n_note < 67) n_note += 12; // G4..F#5 (67..78)
    while (n_note > 78) n_note -= 12;

    // 4th voice: Root by default (pure R-3-5-b7 dominant/minor 7th chord), 9th only at high variation (> 75)
    uint8_t top_ext = (variation > 75) ? n_note : r_note;

    // Candidate 0: Core 7th/9th Voicing (3rd + 7th + 5th + Root/9th)
    candidates[0].notes[0] = t_note;
    candidates[0].notes[1] = s_note;
    candidates[0].notes[2] = f_note;
    candidates[0].notes[3] = top_ext;
    candidates[0].count = (variation <= 30) ? 2 : (variation <= 60) ? 3 : 4;

    // Candidate 1: Root-Anchored 7th Voicing (Root + 3rd + 7th + 5th)
    candidates[1].notes[0] = r_note;
    candidates[1].notes[1] = t_note;
    candidates[1].notes[2] = s_note;
    candidates[1].notes[3] = f_note;
    candidates[1].count = (variation <= 30) ? 2 : (variation <= 60) ? 3 : 4;

    // Candidate 2: B Inversion (7th on bottom + 3rd + 5th + Root/9th)
    candidates[2].notes[0] = (uint8_t)(s_note > 71 ? s_note - 12 : s_note);
    candidates[2].notes[1] = t_note;
    candidates[2].notes[2] = f_note;
    candidates[2].notes[3] = top_ext;
    candidates[2].count = (variation <= 30) ? 2 : (variation <= 60) ? 3 : 4;

    // Candidate 3: Open Organ Voicing (3rd + 5th + 7th + Root/9th)
    candidates[3].notes[0] = t_note;
    candidates[3].notes[1] = f_note;
    candidates[3].notes[2] = s_note;
    candidates[3].notes[3] = top_ext;
    candidates[3].count = (variation <= 30) ? 2 : (variation <= 60) ? 3 : 4;

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

// 10 Independent Bass Line Patterns (16 steps per bar)
static const uint8_t BASS_STYLE_MASKS[NUM_CLASSIC_BASS_STYLES][16] = {
    {127,0,0,0,  108,0,0,0,  118,0,0,0,  108,0,0,0},  // 0: WALKING 4/4 (quarter-note pocket)
    {127,0,88,0, 112,0,86,0, 120,0,88,0, 112,0,86,0}, // 1: SHUFFLE (repeats beat note on &)
    {127,0,98,0, 115,0,98,0, 122,0,98,0, 115,0,98,0}, // 2: BOOGIE BOX (1-3-5-6-b7-6-5-3)
    {127,0,0,0,  0,0,95,0,   118,0,0,0,  105,0,0,0},  // 3: ROOT-5TH (heavy root pocket)
    {127,0,95,0, 0,0,110,0,  120,0,0,0,  105,0,95,0}, // 4: FUNK SYNC (syncopated Memphis pocket)
    {127,0,0,0,  0,0,110,0,  0,0,0,0,    118,0,0,0},  // 5: RUMBA / SWAMP (1 - 2& - 4 clave bass)
    {127,0,0,0,  108,0,0,0,  118,0,0,0,  112,0,96,0}, // 6: GOSPEL WALK (1 - 2 - #2 - 3 walk-up)
    {127,0,90,0, 106,0,90,0, 116,0,90,0, 108,0,90,0}, // 7: SLOW 12/8 ROLLING (1-3-5-b7-5-3)
    {127,0,95,0, 0,0,108,0,  120,0,96,0, 0,0,106,0},  // 8: MOTOWN 8THS (Jamerson syncopations)
    {127,0,0,0,  0,0,115,0,  118,0,0,0,  110,0,95,0}, // 9: SECOND LINE (NOLA street-parade tuba)
    {127,0,96,0, 0,0,114,0,  122,0,98,0, 0,0,110,0},  // 10: FUNK POCKET (Root on 1, b7/octave pop on 2& & 3, pickup on 4&)
    {127,0,0,0,  0,0,0,0,    120,0,0,0,  105,0,96,0}  // 11: COUNTRY 2-BEAT (Root on 1, 5th on 3, walk-up on 4 / 4&)
};

// 12 Independent Keys Comping Styles with 2-Bar A/B Phrasing
static const uint8_t KEYS_STYLE_MASKS[NUM_KEYS_STYLES][2][16] = {
    { // 0: CHARLESTON: Beat 1 + & of Beat 2 (classic open blues pocket)
      {105,0,0,0, 0,0,92,0, 0,0,0,0, 0,0,0,0},
      {105,0,0,0, 0,0,92,0, 0,0,0,0, 0,0,0,0} },
    { // 1: BACKBEAT 2 & 4: leaves Beats 1 & 3 wide open
      {0,0,0,0, 98,0,0,0, 0,0,0,0, 92,0,0,0},
      {0,0,0,0, 98,0,0,0, 0,0,0,0, 92,0,0,0} },
    { // 2: CALL & RESPONSE: 2-bar phrasing (Bar A: 1 + 2&, Bar B: 2& + 4)
      {105,0,0,0, 0,0,92,0, 0,0,0,0, 0,0,0,0},
      {0,0,0,0,   0,0,95,0, 0,0,0,0, 88,0,0,0} },
    { // 3: SPARSE PAD: sustained Beat 1 (+ gentle 4& push on Bar B)
      {95,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0},
      {95,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,78,0} },
    { // 4: CHOP 4/4: driving rock/boogie chop on all 4 beats
      {100,0,0,0, 88,0,0,0, 96,0,0,0, 88,0,0,0},
      {100,0,0,0, 88,0,0,0, 96,0,0,0, 88,0,0,0} },
    { // 5: STRIDE ("Oom-Pah"): Beat 1/3 bounce + heavy Beat 2/4 chord snap
      {92,0,0,0, 114,0,0,0, 90,0,0,0, 114,0,0,0},
      {92,0,0,0, 114,0,0,0, 90,0,0,0, 114,0,82,0} },
    { // 6: BOOGIE 8THS: relentless 8-step bouncing piano ostinato
      {110,0,90,0, 104,0,90,0, 110,0,90,0, 104,0,90,0},
      {110,0,90,0, 104,0,90,0, 110,0,90,0, 104,0,90,0} },
    { // 7: GOSPEL SWELL: Bar A 1 + 4& push, Bar B 1 + 3& + 4& church swell
      {100,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,92,0},
      {102,0,0,0, 0,0,0,0, 0,0,88,0, 0,0,96,0} },
    { // 8: RUMBA MONTUNO: syncopated NOLA/Afro-Cuban 1 - 2& - 4&
      {105,0,0,0, 0,0,96,0, 0,0,0,0, 0,0,94,0},
      {105,0,0,0, 0,0,96,0, 0,0,90,0, 0,0,0,0} },
    { // 9: SLOW 12/8 ARP: rolling triplet broken comping
      {102,0,80,0, 90,0,0,0, 98,0,80,0, 90,0,82,0},
      {102,0,80,0, 90,0,0,0, 98,0,80,0, 90,0,82,0} },
    { // 10: FUNK CLAV: "The One" + syncopated 2& and 4 (clean, open funk pocket)
      {112,0,0,0, 0,0,98,0,  0,0,0,0,   94,0,0,0},
      {0,0,96,0,  104,0,0,0, 0,0,96,0,  0,0,92,0} },
    { // 11: COUNTRY CHOP: crisp backbeat chop on Beats 2 & 4 (+ 3& bounce on Bar B)
      {0,0,0,0,   104,0,0,0, 0,0,0,0,   100,0,0,0},
      {0,0,0,0,   104,0,0,0, 0,0,88,0,  104,0,0,0} }
};

#include "blues_patterns.c"

void arranger_apply_groove_defaults(HarmonyState *hs, PresetConfig *preset, GrooveStyle style) {
    uint8_t idx = (uint8_t)style % NUM_GROOVES;
    preset->groove_style = idx;
    preset->drum_pattern = idx;
    hs->drum_pattern = idx;
    preset->swing = GROOVE_TEMPLATES[idx].swing_ratio;

    switch (idx) {
        case GROOVE_SLOW_12_8:
            preset->bass_style = BASS_RIFF_SLOW_BLUES;
            preset->keys_style = KEYS_STYLE_SLOW_12_8;
            break;
        case GROOVE_TEXAS_SHUFFLE:
            preset->bass_style = BASS_RIFF_TX_SHUFFLE;
            preset->keys_style = KEYS_STYLE_CHARLESTON;
            break;
        case GROOVE_CHICAGO_SWING:
            preset->bass_style = BASS_RIFF_CHI_WALK;
            preset->keys_style = KEYS_STYLE_CALL_RESP;
            break;
        case GROOVE_JAZZ_SHUFFLE:
        case GROOVE_GOSPEL_SWING:
            preset->bass_style = BASS_RIFF_JAZZ_WALK;
            preset->keys_style = (idx == GROOVE_GOSPEL_SWING) ? KEYS_STYLE_GOSPEL_SWELL : KEYS_STYLE_CALL_RESP;
            break;
        case GROOVE_STRAIGHT_ROCK:
            preset->bass_style = BASS_RIFF_ROCK_8;
            preset->keys_style = KEYS_STYLE_BACKBEAT;
            break;
        case GROOVE_TRAIN_BEAT:
        case GROOVE_COUNTRY_TWANG:
            preset->bass_style = BASS_RIFF_COUNTRY;
            preset->keys_style = (idx == GROOVE_COUNTRY_TWANG) ? KEYS_STYLE_COUNTRY_CHOP : KEYS_STYLE_BACKBEAT;
            break;
        case GROOVE_DELTA_STOMP:
            preset->bass_style = BASS_RIFF_LUMP;
            preset->keys_style = KEYS_STYLE_BACKBEAT;
            break;
        case GROOVE_SWAMP_ROCK:
        case GROOVE_BO_DIDDLEY:
            preset->bass_style = BASS_RIFF_SWAMP;
            preset->keys_style = (idx == GROOVE_BO_DIDDLEY) ? KEYS_STYLE_RUMBA_MONTUNO : KEYS_STYLE_BACKBEAT;
            break;
        case GROOVE_SECOND_LINE:
            preset->bass_style = BASS_RIFF_NOLA;
            preset->keys_style = KEYS_STYLE_STRIDE;
            break;
        case GROOVE_NOLA_FUNK:
            preset->bass_style = BASS_RIFF_NOLA;
            preset->keys_style = KEYS_STYLE_FUNK_CLAV;
            break;
        case GROOVE_MEMPHIS_SOUL:
            preset->bass_style = BASS_RIFF_MEMPHIS;
            preset->keys_style = KEYS_STYLE_CALL_RESP;
            break;
        case GROOVE_PURDIE_SHUFFLE:
            preset->bass_style = BASS_RIFF_SLOW_BLUES;
            preset->keys_style = KEYS_STYLE_CALL_RESP;
            break;
        case GROOVE_RHUMBA_BLUES:
            preset->bass_style = BASS_RIFF_RHUMBA;
            preset->keys_style = KEYS_STYLE_RUMBA_MONTUNO;
            break;
        case GROOVE_POCKET_FUNK:
        case GROOVE_BLUES_FUNK:
            preset->bass_style = BASS_RIFF_FUNK;
            preset->keys_style = KEYS_STYLE_FUNK_CLAV;
            break;
        case GROOVE_SLOW_NEO_FUNK:
            preset->bass_style = BASS_RIFF_FUNK;
            preset->keys_style = KEYS_STYLE_PAD;
            break;
        case GROOVE_BOOGIE_WOOGIE:
        default:
            preset->bass_style = BASS_RIFF_BOOGIE;
            preset->keys_style = KEYS_STYLE_BOOGIE_8;
            break;
    }
    hs->bass_style = preset->bass_style;
    hs->keys_style = preset->keys_style;
}

static uint8_t bass_compute_styled_note(ChordContext current, ChordContext next, uint8_t beat, uint8_t sub_idx, uint8_t variation, uint8_t bass_style) {
    uint8_t r = current.root_note % 12;
    uint8_t curr_root = (uint8_t)(40 + ((r + 8) % 12));
    while (curr_root < 40) curr_root += 12;
    while (curr_root > 51) curr_root -= 12;

    uint8_t nr = next.root_note % 12;
    uint8_t next_target = (uint8_t)(40 + ((nr + 8) % 12));
    while (next_target < 40) next_target += 12;
    while (next_target > 51) next_target -= 12;

    uint8_t is_min = (current.quality == CHORD_MIN7 || current.quality == CHORD_DIM7);
    uint8_t third = is_min ? 3 : 4;
    uint8_t fifth = (current.quality == CHORD_DIM7) ? 6 : 7;
    uint8_t note = curr_root;

    if (bass_style == BASS_STYLE_BOOGIE_BOX) {
        // True 8-step blues box: 1, 3, 5, 6, b7, 6, 5, 3
        uint8_t step8 = (uint8_t)((beat * 2) + (sub_idx >= 2 ? 1 : 0)) % 8;
        static const uint8_t BOX_OFFSETS_MAJ[8] = { 0, 4, 7, 9, 10, 9, 7, 4 };
        static const uint8_t BOX_OFFSETS_MIN[8] = { 0, 3, 7, 8, 10, 8, 7, 3 };
        note = curr_root + (is_min ? BOX_OFFSETS_MIN[step8] : BOX_OFFSETS_MAJ[step8]);
    } else if (bass_style == BASS_STYLE_ROOT_FIFTH) {
        if (beat == 2) note = curr_root + fifth;
        else if (beat == 3 && variation > 50) note = curr_root + 10; // b7 pickup
        else note = curr_root;
    } else if (bass_style == BASS_STYLE_SYNCOPATED) {
        if (beat == 0) note = curr_root;
        else if (beat == 1) note = curr_root + 10; // b7 on 2&
        else if (beat == 2) note = curr_root + fifth;
        else note = (sub_idx >= 2) ? (curr_root + third) : curr_root;
    } else if (bass_style == BASS_STYLE_RUMBA) {
        if (beat == 0) note = curr_root;
        else if (beat == 1) note = curr_root + fifth;
        else note = curr_root + (variation > 55 ? 10 : fifth);
    } else if (bass_style == BASS_STYLE_GOSPEL_WALK) {
        // Church walk-up: Root -> 3rd -> 5th -> 6th/b7
        if (beat == 0) note = curr_root;
        else if (beat == 1) note = curr_root + third;
        else if (beat == 2) note = curr_root + fifth;
        else note = curr_root + (is_min ? 10 : 9);
    } else if (bass_style == BASS_STYLE_SLOW_12_8) {
        // Rolling 1 - 3 - 5 - b7 - 5 - 3
        if (beat == 0) note = (sub_idx >= 2) ? (curr_root + third) : curr_root;
        else if (beat == 1) note = (sub_idx >= 2) ? (curr_root + fifth) : (curr_root + third);
        else if (beat == 2) note = (sub_idx >= 2) ? (curr_root + 10) : (curr_root + fifth);
        else note = (sub_idx >= 2) ? (curr_root + third) : (curr_root + fifth);
    } else if (bass_style == BASS_STYLE_MOTOWN) {
        if (beat == 0) note = (sub_idx >= 2) ? (curr_root + fifth) : curr_root;
        else if (beat == 1) note = curr_root + fifth;
        else if (beat == 2) note = (sub_idx >= 2) ? (curr_root + third) : curr_root;
        else note = curr_root + 10; // Solid b7 pocket
    } else if (bass_style == BASS_STYLE_SECOND_LINE) {
        if (beat == 0) note = curr_root;
        else if (beat == 1) note = curr_root + fifth;
        else if (beat == 2) note = curr_root + third;
        else note = (sub_idx >= 2) ? (curr_root + 10) : (curr_root + fifth);
    } else if (bass_style == BASS_STYLE_FUNK_16) {
        // Syncopated Funk Pocket: Root on 1, b7 on 1&, 5th on 2&, Root on 3, b7 on 3&, 5th on 4&
        if (beat == 0) {
            note = (sub_idx >= 2) ? (curr_root + 10) : curr_root;
        } else if (beat == 1) {
            note = curr_root + fifth;
        } else if (beat == 2) {
            note = (sub_idx >= 2) ? (curr_root + 10) : curr_root;
        } else {
            note = curr_root + fifth;
        }
    } else if (bass_style == BASS_STYLE_COUNTRY_2) {
        // Country alternating root-fifth two-beat with chord-tone walk-up (Root - 3rd - 5th)
        if (beat == 0 || beat == 1) {
            note = curr_root;
        } else if (beat == 2) {
            note = curr_root + fifth;
        } else {
            note = (sub_idx >= 2) ? (curr_root + fifth) : (curr_root + third);
        }
    } else if (bass_style == BASS_STYLE_SHUFFLE_REP) {
        // Shuffle repeat: & repeats the current beat's walking note instead of jumping to +7/+12
        return bass_compute_note(current, next, beat, 0, variation);
    } else {
        // WALKING 4/4: clean quarter-note walking line
        return bass_compute_note(current, next, beat, 0, variation);
    }

    while (note < 40) note += 12;
    while (note > 62) note -= 12;
    return note;
}

static uint8_t get_bass_root(uint8_t root_semitone) {
    uint8_t r = (uint8_t)(40 + (((root_semitone % 12) + 8) % 12));
    while (r < 40) r += 12;
    while (r > 51) r -= 12;
    return r;
}

static uint8_t get_keys_root(uint8_t root_semitone) {
    return (uint8_t)(60 + (root_semitone % 12));
}

static void emit_scripted_bass(PerformanceFrame *frame, uint8_t bass_root, int8_t offset, uint8_t vel) {
    int note = (int)bass_root + offset;
    while (note < 36) note += 12;
    while (note > 64) note -= 12;
    frame->bass_trigger = 1;
    frame->bass_note = (uint8_t)note;
    frame->bass_vel = vel;
}

static void emit_scripted_keys_chord(PerformanceFrame *frame, Voicing *last_voicing, ScaleDegree deg, uint8_t root_semitone, uint8_t vel) {
    frame->comp_trigger = 1;
    frame->comp_vel = vel;
    frame->comp_voicing = voice_lead_select(*last_voicing, deg, root_semitone, 0);
    *last_voicing = frame->comp_voicing;
}

static void emit_scripted_keys_9th(PerformanceFrame *frame, Voicing *last_voicing, uint8_t keys_root, int8_t transpose, uint8_t minor_3rd, uint8_t vel) {
    uint8_t r = (uint8_t)((int)keys_root + transpose);
    frame->comp_trigger = 1;
    frame->comp_vel = vel;
    frame->comp_voicing.count = 4;
    frame->comp_voicing.notes[0] = r;
    frame->comp_voicing.notes[1] = (uint8_t)(r + (minor_3rd ? 3 : 4));
    frame->comp_voicing.notes[2] = (uint8_t)(r + 10);
    frame->comp_voicing.notes[3] = (uint8_t)(r + 14);
    *last_voicing = frame->comp_voicing;
}

static void emit_scripted_keys_dyad(PerformanceFrame *frame, Voicing *last_voicing, uint8_t n1, uint8_t n2, uint8_t vel) {
    frame->comp_trigger = 1;
    frame->comp_vel = vel;
    frame->comp_voicing.count = 2;
    frame->comp_voicing.notes[0] = n1;
    frame->comp_voicing.notes[1] = n2;
    *last_voicing = frame->comp_voicing;
}

static void emit_scripted_keys_note(PerformanceFrame *frame, Voicing *last_voicing, uint8_t note, uint8_t vel) {
    frame->comp_trigger = 1;
    frame->comp_vel = vel;
    frame->comp_voicing.count = 1;
    frame->comp_voicing.notes[0] = note;
    *last_voicing = frame->comp_voicing;
}

static void render_turnaround_walkdown_bar(HarmonyState *hs, PerformanceFrame *frame, Voicing *last_voicing, uint8_t step16, uint8_t is_intro) {
    uint8_t br = get_bass_root(hs->root_semitone);
    uint8_t kr = get_keys_root(hs->root_semitone);
    uint8_t is_min = (hs->form_mode == FORM_MINOR_BLUES);
    ScaleDegree root_deg = is_min ? DEG_i7 : DEG_I7;
    if (step16 == 0) {
        emit_scripted_bass(frame, br, 0, 114);
        emit_scripted_keys_chord(frame, last_voicing, root_deg, hs->root_semitone, 108);
        frame->drum_mask = 0x05; frame->drum_vel[0] = 112; frame->drum_vel[2] = 95;
    } else if (step16 == 2) {
        emit_scripted_bass(frame, br, is_intro ? 3 : 4, 102);
        emit_scripted_keys_dyad(frame, last_voicing, (uint8_t)(kr + 10), (uint8_t)(kr + 12), 98);
        frame->drum_mask = 0x04; frame->drum_vel[2] = 75;
    } else if (step16 == 4) {
        emit_scripted_bass(frame, br, is_intro ? 4 : 5, 106);
        emit_scripted_keys_dyad(frame, last_voicing, (uint8_t)(kr + 9), (uint8_t)(kr + 12), 100);
        frame->drum_mask = 0x06; frame->drum_vel[1] = 104; frame->drum_vel[2] = 90;
    } else if (step16 == 8) {
        emit_scripted_bass(frame, br, is_intro ? 5 : 6, 108);
        emit_scripted_keys_dyad(frame, last_voicing, (uint8_t)(kr + 8), (uint8_t)(kr + 12), 102);
        frame->drum_mask = 0x05; frame->drum_vel[0] = 105; frame->drum_vel[2] = 88;
    } else if (step16 == 12) {
        emit_scripted_bass(frame, br, is_intro ? 6 : 7, 112);
        emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 108);
        frame->drum_mask = 0x06; frame->drum_vel[1] = 108; frame->drum_vel[2] = 92;
    }
}

static void render_scripted_intro_frame(HarmonyState *hs, PerformanceFrame *frame, Voicing *last_voicing, uint8_t sub_slot) {
    uint8_t step16 = (uint8_t)(hs->current_beat * 4 + sub_slot);
    uint8_t bar_idx = (hs->intro_total_bars >= hs->count_in_bars_remaining)
                      ? (uint8_t)(hs->intro_total_bars - hs->count_in_bars_remaining) : 0;
    uint8_t br = get_bass_root(hs->root_semitone);
    uint8_t kr = get_keys_root(hs->root_semitone);
    uint8_t is_min = (hs->form_mode == FORM_MINOR_BLUES);
    ScaleDegree root_deg = is_min ? DEG_i7 : DEG_I7;
    ScaleDegree four_deg = is_min ? DEG_iv7 : DEG_IV7;
    IntroMode im = hs->active_intro_style;
    if (im == INTRO_OFF) return;

    if (im == INTRO_CLICK_1BAR) {
        if (sub_slot == 0) {
            frame->drum_mask = 0x08;
            frame->drum_vel[3] = (hs->current_beat == 0) ? 115 : 90;
        }
        return;
    }

    if (im == INTRO_DRONE_1BAR) {
        // Lucinda "Righteously" intro: ONE sustained downbeat drone on Beat 1, hi-hat + stick tick on 2, 3, 4
        if (step16 == 0) {
            emit_scripted_bass(frame, br, 0, 122);
            last_voicing->count = 3;
            last_voicing->notes[0] = kr;
            last_voicing->notes[1] = (uint8_t)(kr + 7);
            last_voicing->notes[2] = (uint8_t)(kr + 12);
            frame->comp_voicing = *last_voicing;
            frame->comp_trigger = 1;
            frame->comp_vel = 116;
            frame->drum_mask = 0x05; // Kick + Hat on Beat 1
            frame->drum_vel[0] = 120;
            frame->drum_vel[2] = 100;
        } else if (sub_slot == 0) {
            frame->drum_mask = 0x0C; // Closed Hi-Hat (0x04) + Stick Tick (0x08) on 2, 3, 4
            frame->drum_vel[2] = 102;
            frame->drum_vel[3] = 96;
        }
        return;
    }

    if (im == INTRO_V7_1BAR) {
        // 1-bar V7 turnaround lick: V9 stab + descending blues lick + chromatic bass walk-up
        if (step16 == 0) {
            emit_scripted_bass(frame, br, 7, 116);
            emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 114);
            frame->drum_mask = 0x05; frame->drum_vel[0] = 115; frame->drum_vel[2] = 100;
        } else if (step16 == 2) {
            emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + 7), 96);
        } else if (step16 == 4) {
            emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + 5), 98);
            frame->drum_mask = 0x06; frame->drum_vel[1] = 104; frame->drum_vel[2] = 88;
        } else if (step16 == 6) {
            emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + 3), 98);
        } else if (step16 == 8) {
            emit_scripted_bass(frame, br, 3, 104);
            emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + 2), 100);
            frame->drum_mask = 0x05; frame->drum_vel[0] = 105; frame->drum_vel[2] = 90;
        } else if (step16 == 10) {
            emit_scripted_bass(frame, br, 4, 108);
            emit_scripted_keys_note(frame, last_voicing, kr, 104);
        } else if (step16 == 12) {
            emit_scripted_bass(frame, br, 5, 112);
            frame->drum_mask = 0x02; frame->drum_vel[1] = 106;
        } else if (step16 == 14) {
            emit_scripted_bass(frame, br, 6, 116);
            frame->drum_mask = 0x03; frame->drum_vel[0] = 95; frame->drum_vel[1] = 114;
        }
        return;
    }

    if (im == INTRO_FUNK_2BAR) {
        if (bar_idx == 0) {
            // Bar 1: Syncopated unison funk riff (1 - 1 - b3 - 3 - 5 - b7 - 1)
            static const int8_t FUNK_OFF[6] = { 0, 0, 3, 4, 7, 10 };
            static const uint8_t FUNK_STEPS[6] = { 0, 2, 6, 8, 10, 14 };
            for (int i = 0; i < 6; i++) {
                if (step16 == FUNK_STEPS[i]) {
                    emit_scripted_bass(frame, br, FUNK_OFF[i], (i == 0) ? 116 : 104);
                    emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + FUNK_OFF[i]), (i == 0) ? 110 : 98);
                }
            }
            if (step16 == 0 || step16 == 6 || step16 == 10) { frame->drum_mask |= 0x01; frame->drum_vel[0] = 110; }
            if (step16 == 4 || step16 == 12) { frame->drum_mask |= 0x02; frame->drum_vel[1] = 106; }
            if (sub_slot == 0 || sub_slot == 2) { frame->drum_mask |= 0x04; frame->drum_vel[2] = 86; }
        } else {
            // Bar 2: V7 syncopated stabs + tight Beat-4 snare pickup
            if (step16 == 0 || step16 == 6 || step16 == 8) {
                emit_scripted_bass(frame, br, (step16 == 6) ? 10 : 7, 112);
                emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 110);
                frame->drum_mask |= 0x05; frame->drum_vel[0] = 110; frame->drum_vel[2] = 92;
            }
            if (step16 == 4) { frame->drum_mask |= 0x02; frame->drum_vel[1] = 102; }
            if (step16 == 12) {
                emit_scripted_bass(frame, br, 7, 108);
                frame->drum_mask = 0x02; frame->drum_vel[1] = 104;
            } else if (step16 == 14) {
                emit_scripted_bass(frame, br, 10, 115);
                frame->drum_mask = 0x03; frame->drum_vel[0] = 96; frame->drum_vel[1] = 116;
            }
        }
        return;
    }

    // INTRO_BAND_4BAR: Bars 0..1 play V7 -> IV7 stabs, Bars 2..3 play the 2-bar turnaround
    uint8_t turn_bar = bar_idx;
    if (im == INTRO_BAND_4BAR) {
        if (bar_idx < 2) {
            ScaleDegree deg = (bar_idx == 0) ? DEG_V7 : four_deg;
            int8_t boff = (bar_idx == 0) ? 7 : 5;
            if (step16 == 0 || step16 == 6 || step16 == 12) {
                emit_scripted_bass(frame, br, (step16 == 12) ? (int8_t)(boff + 3) : boff, 112);
                emit_scripted_keys_chord(frame, last_voicing, deg, hs->root_semitone, 108);
            } else if (step16 == 4 || step16 == 8) {
                emit_scripted_bass(frame, br, (int8_t)(boff + (step16 == 4 ? 4 : 5)), 104);
            }
            if (step16 == 0 || step16 == 8) { frame->drum_mask |= 0x01; frame->drum_vel[0] = 110; }
            if (step16 == 4 || step16 == 12) { frame->drum_mask |= 0x02; frame->drum_vel[1] = 105; }
            if (sub_slot == 0 || sub_slot == 2) { frame->drum_mask |= 0x04; frame->drum_vel[2] = 88; }
            return;
        }
        turn_bar = (uint8_t)(bar_idx - 2);
    }

    // INTRO_TURNAROUND_2BAR (and Bars 2..3 of INTRO_BAND_4BAR)
    if (turn_bar == 0) {
        render_turnaround_walkdown_bar(hs, frame, last_voicing, step16, 1);
    } else {
        // Bar 2: V7 punchy hits + Beat-4 drum pickup into Bar 1
        if (step16 == 0) {
            emit_scripted_bass(frame, br, 7, 115);
            emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 112);
            frame->drum_mask = 0x05; frame->drum_vel[0] = 114; frame->drum_vel[2] = 95;
        } else if (step16 == 4) {
            emit_scripted_bass(frame, br, 4, 104);
            emit_scripted_keys_chord(frame, last_voicing, root_deg, hs->root_semitone, 100);
            frame->drum_mask = 0x06; frame->drum_vel[1] = 104; frame->drum_vel[2] = 90;
        } else if (step16 == 8) {
            emit_scripted_bass(frame, br, 7, 116);
            emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 114);
            frame->drum_mask = 0x07; frame->drum_vel[0] = 115; frame->drum_vel[1] = 105; frame->drum_vel[2] = 100;
        } else if (step16 == 12) {
            frame->drum_mask = 0x02; frame->drum_vel[1] = 102;
        } else if (step16 == 14) {
            emit_scripted_bass(frame, br, 10, 112);
            frame->drum_mask = 0x0A; frame->drum_vel[1] = 112; frame->drum_vel[3] = 95;
        }
    }
}

static void render_scripted_outro_frame(HarmonyState *hs, PerformanceFrame *frame, Voicing *last_voicing, uint8_t sub_slot) {
    uint8_t step16 = (uint8_t)(hs->current_beat * 4 + sub_slot);
    uint8_t br = get_bass_root(hs->root_semitone);
    uint8_t kr = get_keys_root(hs->root_semitone);
    uint8_t is_min = (hs->form_mode == FORM_MINOR_BLUES);
    ScaleDegree root_deg = is_min ? DEG_i7 : DEG_I7;
    ScaleDegree four_deg = is_min ? DEG_iv7 : DEG_IV7;
    uint8_t is_bar1 = (hs->ending_armed == ENDING_BAR_1);

    if (hs->outro_type == OUTRO_CLASSIC_SLIDE) {
        if (is_bar1) {
            render_turnaround_walkdown_bar(hs, frame, last_voicing, step16, 0);
        } else {
            // Bar 2: III -> IV -> #IV -> V walkup + #I9 -> I9 half-step slide pickup!
            if (step16 == 0) {
                emit_scripted_bass(frame, br, 4, 112);
                emit_scripted_keys_chord(frame, last_voicing, root_deg, hs->root_semitone, 108);
                frame->drum_mask = 0x05; frame->drum_vel[0] = 112; frame->drum_vel[2] = 95;
            } else if (step16 == 2) {
                emit_scripted_bass(frame, br, 5, 108);
                emit_scripted_keys_chord(frame, last_voicing, four_deg, hs->root_semitone, 104);
            } else if (step16 == 4) {
                emit_scripted_bass(frame, br, 6, 112);
                emit_scripted_keys_chord(frame, last_voicing, DEG_SHARP_IV_DIM7, hs->root_semitone, 108);
                frame->drum_mask = 0x06; frame->drum_vel[1] = 108; frame->drum_vel[2] = 92;
            } else if (step16 == 8) {
                emit_scripted_bass(frame, br, 7, 118);
                emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 116);
                frame->drum_mask = 0x07; frame->drum_vel[0] = 116; frame->drum_vel[1] = 108; frame->drum_vel[2] = 105;
            } else if (step16 == 14) {
                // #I9 (bII9) slide-down pickup on Beat 4&!
                emit_scripted_bass(frame, br, 1, 120);
                emit_scripted_keys_9th(frame, last_voicing, kr, 1, is_min, 118);
                frame->drum_mask = 0x03; frame->drum_vel[0] = 108; frame->drum_vel[1] = 116;
            }
        }
        return;
    }

    if (hs->outro_type == OUTRO_V_bV_IV_I) {
        if (is_bar1) {
            // Bar 1: V9 -> bV9 -> IV9 chromatic ensemble hits
            if (step16 == 0) {
                emit_scripted_bass(frame, br, 7, 118);
                emit_scripted_keys_9th(frame, last_voicing, kr, 7, 0, 116);
                frame->drum_mask = 0x05; frame->drum_vel[0] = 118; frame->drum_vel[2] = 105;
            } else if (step16 == 6) {
                emit_scripted_bass(frame, br, 6, 116);
                emit_scripted_keys_9th(frame, last_voicing, kr, 6, 0, 114);
                frame->drum_mask = 0x03; frame->drum_vel[0] = 110; frame->drum_vel[1] = 112;
            } else if (step16 == 12) {
                emit_scripted_bass(frame, br, 5, 118);
                emit_scripted_keys_9th(frame, last_voicing, kr, 5, is_min, 116);
                frame->drum_mask = 0x06; frame->drum_vel[1] = 114; frame->drum_vel[2] = 98;
            }
        } else {
            // Bar 2: Unison blues-box tag (1 -> b3 -> 3 -> 5 -> 6 -> b7) + #I9 pickup
            static const int8_t TAG_OFF[6] = { 0, 3, 4, 7, 9, 10 };
            static const uint8_t TAG_STEP[6] = { 0, 2, 4, 8, 10, 12 };
            for (int i = 0; i < 6; i++) {
                if (step16 == TAG_STEP[i]) {
                    emit_scripted_bass(frame, br, TAG_OFF[i], 112);
                    emit_scripted_keys_note(frame, last_voicing, (uint8_t)(kr + TAG_OFF[i]), 110);
                }
            }
            if (step16 == 0 || step16 == 8) { frame->drum_mask |= 0x01; frame->drum_vel[0] = 110; }
            if (step16 == 4 || step16 == 12) { frame->drum_mask |= 0x02; frame->drum_vel[1] = 106; }
            if (step16 == 14) {
                emit_scripted_bass(frame, br, 1, 120);
                emit_scripted_keys_9th(frame, last_voicing, kr, 1, is_min, 118);
                frame->drum_mask = 0x03; frame->drum_vel[0] = 108; frame->drum_vel[1] = 116;
            }
        }
        return;
    }

    if (hs->outro_type == OUTRO_IV_I_TURN) {
        if (is_bar1) {
            // Bar 1: IV7 (beats 1-2) -> #IVdim7 (beats 3-4)
            if (step16 == 0 || step16 == 4) {
                emit_scripted_bass(frame, br, (step16 == 0) ? 5 : 0, 110);
                emit_scripted_keys_chord(frame, last_voicing, four_deg, hs->root_semitone, 108);
            } else if (step16 == 8 || step16 == 12) {
                emit_scripted_bass(frame, br, 6, 112);
                emit_scripted_keys_chord(frame, last_voicing, DEG_SHARP_IV_DIM7, hs->root_semitone, 110);
            }
            if (step16 == 0 || step16 == 8) { frame->drum_mask |= 0x01; frame->drum_vel[0] = 110; }
            if (step16 == 4 || step16 == 12) { frame->drum_mask |= 0x02; frame->drum_vel[1] = 105; }
            if (sub_slot == 0 || sub_slot == 2) { frame->drum_mask |= 0x04; frame->drum_vel[2] = 88; }
        } else {
            // Bar 2: I7 -> VI7 -> ii7 -> V7 -> #I9 -> I9
            if (step16 == 0) {
                emit_scripted_bass(frame, br, 7, 112);
                emit_scripted_keys_chord(frame, last_voicing, root_deg, hs->root_semitone, 110);
                frame->drum_mask = 0x05; frame->drum_vel[0] = 112; frame->drum_vel[2] = 95;
            } else if (step16 == 4) {
                emit_scripted_bass(frame, br, 9, 110);
                emit_scripted_keys_chord(frame, last_voicing, DEG_VI7, hs->root_semitone, 108);
                frame->drum_mask = 0x06; frame->drum_vel[1] = 106; frame->drum_vel[2] = 90;
            } else if (step16 == 8) {
                emit_scripted_bass(frame, br, 2, 112);
                emit_scripted_keys_chord(frame, last_voicing, DEG_ii7, hs->root_semitone, 110);
                frame->drum_mask = 0x05; frame->drum_vel[0] = 110; frame->drum_vel[2] = 92;
            } else if (step16 == 12) {
                emit_scripted_bass(frame, br, 7, 116);
                emit_scripted_keys_chord(frame, last_voicing, DEG_V7, hs->root_semitone, 114);
                frame->drum_mask = 0x06; frame->drum_vel[1] = 110; frame->drum_vel[2] = 96;
            } else if (step16 == 14) {
                emit_scripted_bass(frame, br, 1, 120);
                emit_scripted_keys_9th(frame, last_voicing, kr, 1, is_min, 118);
                frame->drum_mask = 0x03; frame->drum_vel[0] = 108; frame->drum_vel[1] = 116;
            }
        }
        return;
    }

    // OUTRO_QUICK_1BAR (1 bar + stinger): V9 -> bV9 -> IV9 -> #I9 -> I9
    if (step16 == 0) {
        emit_scripted_bass(frame, br, 7, 118);
        emit_scripted_keys_9th(frame, last_voicing, kr, 7, 0, 116);
        frame->drum_mask = 0x05; frame->drum_vel[0] = 116; frame->drum_vel[2] = 100;
    } else if (step16 == 4) {
        emit_scripted_bass(frame, br, 6, 114);
        emit_scripted_keys_9th(frame, last_voicing, kr, 6, 0, 112);
        frame->drum_mask = 0x06; frame->drum_vel[1] = 110; frame->drum_vel[2] = 95;
    } else if (step16 == 8) {
        emit_scripted_bass(frame, br, 5, 116);
        emit_scripted_keys_9th(frame, last_voicing, kr, 5, is_min, 114);
        frame->drum_mask = 0x05; frame->drum_vel[0] = 114; frame->drum_vel[2] = 96;
    } else if (step16 == 14) {
        emit_scripted_bass(frame, br, 1, 120);
        emit_scripted_keys_9th(frame, last_voicing, kr, 1, is_min, 118);
        frame->drum_mask = 0x03; frame->drum_vel[0] = 108; frame->drum_vel[1] = 116;
    }
}

void arranger_generate_frame(HarmonyState *hs, const PresetConfig *preset, PerformanceFrame *frame, Voicing *last_voicing) {
    frame->bass_trigger = 0;
    frame->bass_vel = 0;
    frame->comp_trigger = 0;
    frame->comp_vel = 0;
    frame->drum_mask = 0;
    for (int d = 0; d < NUM_DRUM_VOICES; d++) frame->drum_vel[d] = 0;
    frame->bass_gate_ticks = 0;
    frame->bass_ghost = 0;

    if (!hs->is_playing) return;

    /* The beat being played. The classic (pre-2.3) beats are a hat line and a swing over one shared kick and snare
     * (classic_tmpl: which of the old templates); the groove beats have their own lanes (blues_patterns.c) */
    static const uint8_t CLASSIC_TMPL[NUM_DRUM_PATTERNS - NUM_GROOVES] = {
        GROOVE_SLOW_12_8, GROOVE_TEXAS_SHUFFLE, GROOVE_BO_DIDDLEY, GROOVE_STRAIGHT_ROCK, GROOVE_POCKET_FUNK,
        GROOVE_BLUES_FUNK, GROOVE_TRAIN_BEAT, GROOVE_RHUMBA_BLUES, GROOVE_DELTA_STOMP, GROOVE_JAZZ_SHUFFLE
    };
    uint8_t dpat = hs->drum_pattern % NUM_DRUM_PATTERNS;
    uint8_t classic_drums = (dpat >= NUM_GROOVES);
    const GrooveTemplate *gt = &GROOVE_TEMPLATES[classic_drums ? CLASSIC_TMPL[dpat - NUM_GROOVES] : dpat];
    uint8_t is_12_8 = (gt->style == GROOVE_SLOW_12_8);
    uint8_t sub_slot = 0xFF;

    if (is_12_8) {
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

    if (hs->ending_armed == ENDING_STINGER) {
        // Final resolution stinger: unified Root (I9 / i7) hit on Beat 1, then stop and ring out
        if (hs->current_beat == 0 && hs->current_subdiv == 0) {
            uint8_t br = get_bass_root(hs->root_semitone);
            uint8_t kr = get_keys_root(hs->root_semitone);
            uint8_t is_min = (hs->form_mode == FORM_MINOR_BLUES);

            frame->bass_trigger = 1;
            frame->bass_note = br;
            frame->bass_vel = 127;

            emit_scripted_keys_9th(frame, last_voicing, kr, 0, is_min, 122);

            frame->drum_mask = 0x07; // Kick + Snare + Cymbal crash
            frame->drum_vel[0] = 127;
            frame->drum_vel[1] = 115;
            frame->drum_vel[2] = 112;

            hs->is_playing = 0;
            hs->ending_armed = ENDING_NONE;
            hs->ui_dirty = 1;
        }
        return;
    }

    if (hs->ending_armed == ENDING_BAR_1 || hs->ending_armed == ENDING_BAR_2) {
        if (sub_slot != 0xFF) {
            render_scripted_outro_frame(hs, frame, last_voicing, sub_slot);
        }
        return;
    }

    if (hs->count_in_bars_remaining > 0) {
        if (sub_slot != 0xFF) {
            render_scripted_intro_frame(hs, frame, last_voicing, sub_slot);
        }
        return;
    }

    if (hs->stop_time_counter > 0) {
        if (hs->current_beat > 0 || hs->current_subdiv > 0) return;
    }

    {
        int8_t j_vel = get_hmnz_jitter(hs->drums_variation);
        uint8_t riff_style = hs->bass_style % NUM_BASS_STYLES;
        if (riff_style >= NUM_CLASSIC_BASS_STYLES) bass_riff_generate(hs, preset, frame, riff_style, is_12_8, j_vel);
        if (!classic_drums) drums_generate(hs, preset, frame, j_vel);
    }

    if (sub_slot != 0xFF) {
        uint8_t step16 = (uint8_t)((hs->current_beat * 4) + sub_slot);
        BarSlot curr_slot = harmony_get_current_bar_slot(hs);
        ScaleDegree active_degree = (hs->current_beat >= 2 && curr_slot.has_split) ? curr_slot.secondary : curr_slot.primary;
        int8_t j_vel = get_hmnz_jitter(hs->drums_variation);
        uint8_t b_full = hs->bass_fullness;
        uint8_t k_full = hs->keys_fullness;
        uint8_t d_full = hs->drums_fullness;
        uint8_t b_var  = hs->bass_variation;
        uint8_t k_var  = hs->keys_variation;
        uint8_t d_var  = hs->drums_variation;
        uint8_t b_style = hs->bass_style % NUM_BASS_STYLES;
        uint8_t classic_bass = (b_style < NUM_CLASSIC_BASS_STYLES);
        uint8_t k_style = hs->keys_style % NUM_KEYS_STYLES;

        // Bass engine trigger: per-instrument Fullness & Spice
        uint8_t bass_vel_mask = classic_bass ? BASS_STYLE_MASKS[b_style][step16 % 16] : 0;
        uint8_t bass_hit = (bass_vel_mask > 0);
        if (!classic_bass) b_full = 50;                                 /* (the riff line has played above) */
        if (b_full < 20) {
            bass_hit = (hs->current_beat == 0 && sub_slot == 0); // Lowest fullness: 1 note per measure (Root)
        } else if (b_full < 40) {
            bass_hit = ((hs->current_beat == 0 || hs->current_beat == 2) && sub_slot == 0); // Sparse: 1 - 5
        } else if (b_full > 85 && hs->current_beat == 3 && sub_slot == 2) {
            bass_hit = 1;
        }
        if (bass_hit && classic_bass) {
            frame->bass_trigger = 1;
            int base_v = (bass_vel_mask > 0) ? bass_vel_mask : 86;
            int bv = base_v + j_vel + ((b_full > 75) ? 8 : (b_full < 40) ? -8 : 0);
            frame->bass_vel = (uint8_t)((bv < 1) ? 1 : (bv > 127) ? 127 : bv);

            BarSlot next_slot = harmony_get_next_bar_slot(hs);
            ScaleDegreeInfo curr_info = harmony_get_degree_info(active_degree);
            ScaleDegreeInfo next_info = harmony_get_degree_info(next_slot.primary);
            ChordContext c_curr = { (uint8_t)((hs->root_semitone + curr_info.semitone_offset) % 12), curr_info.quality };
            ChordContext c_next = { (uint8_t)((hs->root_semitone + next_info.semitone_offset) % 12), next_info.quality };
            if (b_full < 40) {
                // Sparse pocket: always 1 (or 1 - 5), never 1 - 6 - 1 - 6
                uint8_t fifth_off = (c_curr.quality == CHORD_DIM7) ? 6 : 7;
                frame->bass_note = bass_compute_styled_note(c_curr, c_next, (hs->current_beat == 0) ? 0 : 2, 0, 0, BASS_STYLE_ROOT_FIFTH);
                if (hs->current_beat == 2 && fifth_off == 6) frame->bass_note--;
            } else if (b_var <= 30) {
                // Low bass variation: clean root-5th pocket regardless of rhythm
                frame->bass_note = bass_compute_styled_note(c_curr, c_next, hs->current_beat, sub_slot, 0, BASS_STYLE_ROOT_FIFTH);
            } else {
                frame->bass_note = bass_compute_styled_note(c_curr, c_next, hs->current_beat, sub_slot, b_var, b_style);
            }
        }

        // Comping engine trigger: per-instrument Fullness & Spice
        uint8_t bar_ab = hs->current_bar & 1u;
        uint8_t comp_vel_mask = KEYS_STYLE_MASKS[k_style][bar_ab][step16 % 16];
        uint8_t comp_hit = (comp_vel_mask > 0);
        if (k_full < 25) {
            comp_hit = (hs->current_beat == 1 && sub_slot == 0);
        }
        if (comp_hit || hs->manual_stab_pending) {
            frame->comp_trigger = 1;
            int base_cv = (comp_vel_mask > 0) ? comp_vel_mask : 85;
            if (hs->manual_stab_pending) base_cv = 120;
            int cv = base_cv + j_vel + ((k_full > 75) ? 10 : (k_full < 35) ? -18 : 0);
            frame->comp_vel = (uint8_t)((cv < 1) ? 1 : (cv > 127) ? 127 : cv);
            frame->comp_voicing = voice_lead_select(*last_voicing, active_degree, hs->root_semitone, k_var);
            *last_voicing = frame->comp_voicing;
            hs->manual_stab_pending = 0;
        }

        if (!classic_drums) return;

        // Drum channels: Hi-hat / Ride (gated by Drums Fullness: <20 none, 20..44 quarter-notes, >=45 full pattern)
        if (gt->drum_hat_mask[step16 % 16] > 0 && d_full >= 20) {
            if (d_full >= 45 || sub_slot == 0) {
                frame->drum_mask |= 0x04;
                int dv = gt->drum_hat_mask[step16 % 16] + j_vel + ((d_full < 45) ? -22 : (d_full > 75) ? 6 : 0);
                frame->drum_vel[2] = (uint8_t)((dv < 1) ? 1 : (dv > 127) ? 127 : dv);
            }
        }

        // Kick Drum: Beat 1 downbeat (d_full >= 20), Beat 3 (d_full >= 55), plus syncopated pushes
        if (d_full >= 20 && hs->current_subdiv == 0 && (hs->current_beat == 0 || (d_full >= 55 && hs->current_beat == 2))) {
            frame->drum_mask |= 0x01;
            int dv0 = 112 + j_vel + ((d_full > 75) ? 10 : (d_full < 45) ? -14 : 0);
            frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
        }
        // Syncopated funk / soul kick pushes on 2& and 4& (or from groove template)
        if (d_full >= 45 && sub_slot == 2) {
            uint8_t is_funk = (gt->style >= GROOVE_POCKET_FUNK && gt->style <= GROOVE_SLOW_NEO_FUNK);
            if ((is_funk && (hs->current_beat == 1 || hs->current_beat == 3)) ||
                (d_var > 65 && (hs->current_beat == 3 || (d_full > 75 && hs->current_beat == 1)))) {
                frame->drum_mask |= 0x01;
                int dv0 = (is_funk ? 104 : 96) + j_vel;
                frame->drum_vel[0] = (uint8_t)((dv0 < 1) ? 1 : (dv0 > 127) ? 127 : dv0);
            }
        }

        // Snare / Rimshot: Backbeat on Beats 2 and 4 (current_beat == 1 || 3)
        if ((hs->current_beat == 1 || hs->current_beat == 3) && hs->current_subdiv == 0) {
            if (d_full < 20) {
                frame->drum_mask |= 0x08; // Ultra-sparse: rimshot only on 2 & 4
                int dv3 = 82 + j_vel;
                frame->drum_vel[3] = (uint8_t)((dv3 < 1) ? 1 : (dv3 > 127) ? 127 : dv3);
            } else {
                frame->drum_mask |= 0x02;
                int dv1 = 104 + j_vel + ((d_full > 75) ? 10 : (d_full < 45) ? -16 : 0);
                frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
            }
        }
        // Snare ghost notes (controlled by Drums Spice & Fullness, plus Country train snare)
        if (d_full >= 40 && sub_slot == 2) {
            if (gt->style == GROOVE_COUNTRY_TWANG || gt->style == GROOVE_TRAIN_BEAT) {
                frame->drum_mask |= 0x02;
                int dv1 = 58 + j_vel;
                frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
            } else if (d_var >= 26 && (hs->current_beat == 2 || (d_var > 65 && hs->current_beat == 0))) {
                frame->drum_mask |= 0x02;
                int dv1 = 48 + j_vel + ((d_var > 65) ? 15 : 0);
                frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
            }
        }

        // Automatic phrase-end drum fills: Bar 12 (d_var > 25), Bar 4 & 8 (d_var > 65), or Outro Armed
        uint8_t is_phrase_end = (hs->current_bar == 11) || (d_var > 65 && (hs->current_bar == 3 || hs->current_bar == 7));
        if ((is_phrase_end && d_full >= 25 && d_var > 25) || hs->fill_pending || hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
            if (hs->current_beat == 3) {
                uint8_t is_turnaround_bar = (hs->current_bar == 11 || hs->fill_pending || hs->ending_armed == ENDING_ARMED_NEXT_BAR);
                if (sub_slot == 0) {
                    frame->drum_mask |= 0x02;
                    int dv1 = (is_turnaround_bar ? 105 : 88) + j_vel;
                    frame->drum_vel[1] = (uint8_t)((dv1 < 1) ? 1 : (dv1 > 127) ? 127 : dv1);
                } else if (sub_slot == 2) {
                    frame->drum_mask |= 0x03;
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
        /* classic beats at a high variation: the hat opens on the last off-beat of the bar */
        if (hs->variation > 75 && (frame->drum_mask & (1u << DV_HAT)) && hs->current_beat == 3 && sub_slot == 2) {
            frame->drum_mask = (uint16_t)((frame->drum_mask & ~(1u << DV_HAT)) | (1u << DV_HAT_OPEN));
            frame->drum_vel[DV_HAT_OPEN] = frame->drum_vel[DV_HAT];
        }
    }
}
