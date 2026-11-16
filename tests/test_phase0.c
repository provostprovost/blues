#include <stdio.h>
#include <assert.h>
#include "app_config.h"
#include "blues_types.h"
#include "hal_adapter.h"

// Static size assertions as required by Dinghy storage architecture
_Static_assert(sizeof(PresetConfig) * NUM_PRESET_SLOTS <= 4096, "Preset bank must fit within 4096 B sector");
_Static_assert(sizeof(GlobalSettings) <= 256, "Global settings must fit within sector allocation");

int main(void) {
    printf("[Phase 0] Running TB-1 Contract & Memory Gate Verification...\n");

    // 1. Check constants & identities
    printf("  - App Name: %s\n", APP_NAME);
    printf("  - Package ID: %s\n", FELUCCA_ID);
    printf("  - Sample Rate: %d Hz\n", AUDIO_SAMPLE_RATE);
    printf("  - Block Size: %d frames\n", AUDIO_BLOCK_SIZE);

    // 2. Struct size checks
    printf("  - sizeof(PresetConfig): %zu bytes\n", sizeof(PresetConfig));
    printf("  - 16 Presets total: %zu bytes (limit 4096 B)\n", sizeof(PresetConfig) * NUM_PRESET_SLOTS);
    printf("  - sizeof(GlobalSettings): %zu bytes (limit 256 B)\n", sizeof(GlobalSettings));
    printf("  - sizeof(HarmonyState): %zu bytes\n", sizeof(HarmonyState));

    assert(sizeof(PresetConfig) * NUM_PRESET_SLOTS <= 4096);
    assert(sizeof(GlobalSettings) <= 256);

    // 3. Verify mock HAL functions in STANDALONE_TEST
    hal_fill_rect(0, 0, 240, 240, COL_BLACK);
    hal_draw_text(10, 10, "TB-1 Phase 0 OK", COL_WHITE, COL_BLACK);
    assert(hal_millis() == 0);
    assert(hal_lcd_idle() == 1);

    // 4. Test math helper in mock HAL
    int16_t s0 = hal_sine_q15(0);
    int16_t s90 = hal_sine_q15(1u << 30);
    (void)s0;
    (void)s90;

    printf("[Phase 0] All contract and memory assertions passed successfully!\n");
    return 0;
}
