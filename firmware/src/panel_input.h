#ifndef PANEL_INPUT_H
#define PANEL_INPUT_H

#include <stdint.h>
#include "blues_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void panel_init(void);
void panel_poll_inputs(HarmonyState *hs, PresetConfig *presets, uint8_t *active_preset_idx, uint32_t ms_now, uint8_t *ui_screen);
void panel_handle_midi_in(HarmonyState *hs, uint8_t status, uint8_t d1, uint8_t d2);
void panel_update_leds(const HarmonyState *hs, uint8_t active_preset_idx);
uint8_t panel_check_autosave_needed(uint32_t ms_now);
void panel_clear_autosave_needed(void);

#ifdef __cplusplus
}
#endif

#endif // PANEL_INPUT_H
