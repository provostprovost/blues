#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <stdint.h>
#include "blues_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_render_splash(void);
void ui_render(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t screen_id, uint8_t force_redraw);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREENS_H
