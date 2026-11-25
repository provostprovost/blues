#ifndef BLUES_HARMONY_H
#define BLUES_HARMONY_H

#include "blues_types.h"

ScaleDegreeInfo harmony_get_degree_info(ScaleDegree deg);
void harmony_init(HarmonyState *hs);
uint8_t harmony_form_bars(FormMode mode);
void harmony_set_heat_level(HarmonyState *hs, PresetConfig *p, uint8_t level, uint8_t trigger_fill);
IntroMode harmony_resolve_intro_style(IntroMode mode, GrooveStyle groove);
uint8_t harmony_intro_bar_count(IntroMode concrete_mode);
void harmony_start_playback(HarmonyState *hs, GrooveStyle groove, uint8_t force_intro);
void harmony_tick_subdivision(HarmonyState *hs);
BarSlot harmony_get_current_bar_slot(const HarmonyState *hs);
BarSlot harmony_get_next_bar_slot(const HarmonyState *hs);
BarSlot harmony_get_grid_slot(const HarmonyState *hs, uint8_t bar_idx);
void seq_advance_bar(HarmonyState *hs);

#endif // BLUES_HARMONY_H
