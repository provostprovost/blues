#ifndef HAL_ADAPTER_H
#define HAL_ADAPTER_H

#include <stdint.h>

// RGB565 Color Macros & Palette
#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))
#define COL_BLACK       RGB565(0, 0, 0)
#define COL_WHITE       RGB565(255, 255, 255)
#define COL_AMBER       RGB565(255, 180, 0)
#define COL_CYAN        RGB565(0, 220, 255)
#define COL_GREEN       RGB565(40, 230, 80)
#define COL_RED         RGB565(240, 60, 60)
#define COL_DARKGRAY    RGB565(40, 44, 52)
#define COL_MIDGRAY     RGB565(90, 96, 110)

#ifndef RGB
#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
#endif
#ifndef C_BLACK
#define C_BLACK 0x0000u
#endif
#ifndef C_WHITE
#define C_WHITE 0xFFFFu
#endif

// Encoder Identifiers (matching Dinghy / M-VAVE FM-1)
#ifndef ENC_SELECT
#define ENC_SELECT      0
#define ENC_ALGORITHM   1
#define ENC_KNOB1       2
#define ENC_KNOB2       3
#define ENC_KNOB3       4
#define ENC_KNOB4       5
#define ENC_PRESETS     6
#endif
#define ENC_ALGO        ENC_ALGORITHM
#define ENC_PRESET      ENC_PRESETS

// Button Identifiers (matching physical M-VAVE FM-1 & Dinghy platform.c)
#ifndef BTN_OCT_DOWN
#define BTN_OCT_DOWN    0
#define BTN_OCT_UP      1
#define BTN_FX          2
#define BTN_SEL         3
#define BTN_ENV         4
#define BTN_LFO         5
#define BTN_EDIT        6
#define BTN_GLO         7
#define BTN_HOME        8
#define BTN_SAVE        9
#define BTN_ARP         10
#define BTN_SEQ         11
#define BTN_PLAY        12
#define BTN_REC         13
#endif

#ifndef KEY_LED
#define KEY_LED(k)      (14u + (k))
#endif

#define LED_OFF         0
#define LED_ON          1

#ifdef STANDALONE_TEST
#include <stdio.h>
#include <string.h>

static uint16_t g_mock_fb[240 * 240];
static inline uint32_t hal_millis(void) { return 0; }
static inline uint32_t hal_ticks(void) { return 0; }
#define HAL_TICKS_PER_US 24

static inline void hal_poll(void) {}

static inline int16_t hal_sine_q15(uint32_t phase) {
    int32_t x = (int32_t)phase >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return (int16_t)(y > 32767 ? 32767 : y < -32767 ? -32767 : y);
}

static inline uint8_t hal_lcd_idle(void) { return 1; }

static inline void hal_fill_rect(int x, int y, int w, int h, uint16_t col) {
    for (int ry = y; ry < y + h && ry < 240; ry++) {
        for (int rx = x; rx < x + w && rx < 240; rx++) {
            if (rx >= 0 && ry >= 0) g_mock_fb[ry * 240 + rx] = col;
        }
    }
}

static inline void hal_draw_text(int x, int y, const char *s, uint16_t fg, uint16_t bg) {
    (void)bg;
    int cx = x;
    while (*s && cx < 240) {
        hal_fill_rect(cx, y, 6, 8, fg);
        cx += 7;
        s++;
    }
}

static inline void hal_draw_text_box(int x, int y, int w, int font_sz, const char *s, uint16_t fg, int align) {
    (void)w; (void)font_sz; (void)align;
    hal_draw_text(x, y, s, fg, 0);
}

static inline void hal_dump_ppm(const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) return;
    fprintf(f, "P6\n240 240\n255\n");
    for (int i = 0; i < 240 * 240; i++) {
        uint16_t p = g_mock_fb[i];
        uint8_t r = ((p >> 11) & 0x1F) << 3;
        uint8_t g = ((p >> 5) & 0x3F) << 2;
        uint8_t b = (p & 0x1F) << 3;
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

static inline uint32_t hal_get_note_edges(void) { return 0; }
static inline uint16_t hal_get_btn_edges(void) { return 0; }
static inline uint16_t hal_get_btns_mask(void) { return 0; }
static inline uint32_t hal_get_keys_mask(void) { return 0; }
static inline uint8_t  hal_get_glo_held(void) { return 0; }
static inline uint8_t  hal_get_zone1_held(void) { return 0; }
static inline int8_t   hal_get_encoder_delta(uint8_t enc) { (void)enc; return 0; }
static inline void     hal_set_btn_led(uint8_t idx, uint8_t level) { (void)idx; (void)level; }
static inline void     hal_set_key_led(uint8_t idx, uint8_t level) { (void)idx; (void)level; }
static inline void     hal_set_key_led_glow(uint8_t idx, uint8_t level) { (void)idx; (void)level; }
static inline int      hal_midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2) { (void)st; (void)d1; (void)d2; return 0; }
static inline void     hal_midi_send(uint8_t status, uint8_t d1, uint8_t d2) { (void)status; (void)d1; (void)d2; }
static inline int      hal_blob_save(uint8_t slot, const void *data, uint16_t len) { (void)slot; (void)data; (void)len; return 0; }
static inline int      hal_blob_load(uint8_t slot, void *data, uint16_t len) { (void)slot; (void)data; (void)len; return -1; }
static inline int      hal_settings_save(const void *data, uint16_t len) { (void)data; (void)len; return 0; }
static inline int      hal_settings_load(void *data, uint16_t len) { (void)data; (void)len; return -1; }

#else

// Target build running inside Dinghy framework (platform.c already included in translation unit)
#if !BOARD_SMK37 && !defined(DINGHY_HOST)
#include "fm1_input.h"
#endif

static inline uint32_t hal_millis(void) { return fm1_ms; }
#ifdef DINGHY_HOST
static inline uint32_t hal_ticks(void) { return fm1_ms * 1000u; }
#define HAL_TICKS_PER_US 1u
#else
static inline uint32_t hal_ticks(void) { return fm1_ticks(); }
#define HAL_TICKS_PER_US FM1_TICKS_PER_US
#endif

static inline void     hal_poll(void) { platform_poll(); }
static inline int16_t  hal_sine_q15(uint32_t phase) {
    int32_t x = (int32_t)phase >> 16, ax = x < 0 ? -x : x, y, ay;
    y = (x * (32768 - ax)) >> 13;
    ay = y < 0 ? -y : y;
    y += ((((y * ay) >> 15) - y) * 7373) >> 15;
    return (int16_t)(y > 32767 ? 32767 : y < -32767 ? -32767 : y);
}
static inline uint8_t  hal_lcd_idle(void) { return 1; }
static inline void     hal_fill_rect(int x, int y, int w, int h, uint16_t col) { lcd_fill(x, y, w, h, col); }
static inline void     hal_draw_text(int x, int y, const char *s, uint16_t fg, uint16_t bg) { (void)bg; draw_text_box(x, y, 240 - x, &FONT_S, s, fg, 0); }
static inline void     hal_draw_text_box(int x, int y, int w, int font_sz, const char *s, uint16_t fg, int align) {
    draw_text_box(x, y, w, font_sz ? &FONT_L : &FONT_S, s, fg, align);
}
static inline void     hal_dump_ppm(const char *filename) { (void)filename; }

static inline uint32_t hal_get_note_edges(void) { return fm1_input_note_edges(); }
static inline uint16_t hal_get_btn_edges(void) {
    uint32_t rel = 0;
    return (uint16_t)fm1_input_edges(&rel);
}
static inline uint16_t hal_get_btns_mask(void) { return (uint16_t)fm1_in.buttons; }
static inline uint32_t hal_get_keys_mask(void) { return fm1_in.notes; }
static inline uint8_t  hal_get_glo_held(void) { return (fm1_in.buttons & (1 << BTN_GLO)) != 0; }
static inline uint8_t  hal_get_zone1_held(void) { return (fm1_in.notes & 0x0FFF) != 0; }
static inline int8_t   hal_get_encoder_delta(uint8_t enc) { return fm1_enc_take(enc); }
static inline void     hal_set_btn_led(uint8_t idx, uint8_t level) { fm1_led_key(idx, level > 0); }
static inline void     hal_set_key_led(uint8_t idx, uint8_t level) { fm1_led_key(KEY_LED(idx), level > 0); }
static inline void     hal_set_key_led_glow(uint8_t idx, uint8_t level) { fm1_led_key_glow(KEY_LED(idx), level > 0); }
static inline int      hal_midi_get(uint8_t *st, uint8_t *d1, uint8_t *d2) { return midi_get(st, d1, d2); }
static inline void     hal_midi_send(uint8_t status, uint8_t d1, uint8_t d2) { midi_send(status, d1, d2); }
static inline int      hal_blob_save(uint8_t slot, const void *data, uint16_t len) { return blob_save(slot, data, len); }
static inline int      hal_blob_load(uint8_t slot, void *data, uint16_t len) { return blob_load(slot, data, len); }
static inline int      hal_settings_save(const void *data, uint16_t len) { return settings_save(data, len); }
static inline int      hal_settings_load(void *data, uint16_t len) { return settings_load(data, len); }

#endif

#endif // HAL_ADAPTER_H
