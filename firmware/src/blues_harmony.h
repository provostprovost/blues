#ifndef BLUES_HARMONY_H
#define BLUES_HARMONY_H

#include "blues_types.h"

ScaleDegreeInfo harmony_get_degree_info(ScaleDegree deg);
void harmony_init(HarmonyState *hs);
void harmony_tick_subdivision(HarmonyState *hs);
BarSlot harmony_get_current_bar_slot(const HarmonyState *hs);
BarSlot harmony_get_next_bar_slot(const HarmonyState *hs);
BarSlot harmony_get_grid_slot(const HarmonyState *hs, uint8_t bar_idx);
void seq_advance_bar(HarmonyState *hs);

#endif // BLUES_HARMONY_H
