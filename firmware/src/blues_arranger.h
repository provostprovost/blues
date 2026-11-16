#ifndef BLUES_ARRANGER_H
#define BLUES_ARRANGER_H

#include "blues_types.h"
#include "blues_harmony.h"

typedef struct {
    uint8_t bass_note;
    uint8_t bass_trigger;
    uint8_t bass_vel;
    Voicing comp_voicing;
    uint8_t comp_trigger;
    uint8_t comp_vel;
    uint8_t drum_mask;
    uint8_t drum_vel[4];
} PerformanceFrame;

typedef struct {
    GrooveStyle style;
    uint8_t     swing_ratio;
    uint8_t     bass_mask[16];
    uint8_t     comp_mask[16];
    uint8_t     drum_hat_mask[16];
} GrooveTemplate;

uint8_t bass_compute_note(ChordContext current, ChordContext next, uint8_t beat, uint8_t sub_idx, uint8_t variation);
Voicing voice_lead_select(Voicing prev_voicing, ScaleDegree degree, uint8_t root_semitone, uint8_t variation);
void arranger_apply_groove_defaults(HarmonyState *hs, PresetConfig *preset, GrooveStyle style);
void arranger_generate_frame(HarmonyState *hs, const PresetConfig *preset, PerformanceFrame *frame, Voicing *last_voicing);

#endif // BLUES_ARRANGER_H
