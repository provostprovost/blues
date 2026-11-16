/* SPDX-License-Identifier: GPL-3.0-only */
/* TexasBlues-1 (TB-1) Firmware for M-VAVE FM-1
 * Phase 3.5: Hardware Checkpoint 2 - Real Silicon Audio DSP & Groove Engine
 */

static void app_idle(void);
#define LCD_IDLE app_idle
#include "platform.c"

#include "blues_types.h"
#include "blues_harmony.h"
#include "blues_arranger.h"
#include "dsp_voices.h"
#include "hal_adapter.h"

#include "blues_harmony.c"
#include "blues_arranger.c"
#include "dsp_voices.c"

#define KEY0 53u

static HarmonyState g_harmony;
static PresetConfig g_preset;
static Voicing      g_last_voicing;
static uint32_t     g_sample_acc = 2646000;
static uint32_t     g_last_redraw = 0;
static uint8_t      g_last_bar = 0xFF;
static uint8_t      g_last_beat = 0xFF;
static uint8_t      g_prev_playing = 0xFF;
static uint8_t      g_prev_algo = 0xFF;
static uint8_t      g_prev_key = 0xFF;

static const char *NOTE_NAMES[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
static const char *ALGO_NAMES[4] = { "Rhodes FM", "Bright EP", "B3 Organ", "Clavinet" };

static void app_idle(void) {
    /* Called during screen DMA refresh */
}

/* Audio ISR (256 stereo frames per block, ~5.8 ms budget) */
static void app_audio(int32_t *out, uint32_t n) {
    if (!g_harmony.is_playing) {
        dsp_all_notes_off();
        dsp_render_block(out, n, &g_preset);
        return;
    }

    uint32_t bpm_valid = (g_preset.bpm < 40) ? 112 : g_preset.bpm;
    for (uint32_t f = 0; f < n; f++) {
        g_sample_acc += bpm_valid * 24;
        if (g_sample_acc >= 2646000) {
            g_sample_acc -= 2646000;
            PerformanceFrame frame;
            arranger_generate_frame(&g_harmony, &g_preset, &frame, &g_last_voicing);
            if (frame.bass_trigger) {
                dsp_note_on_bass(frame.bass_note, frame.bass_vel, g_preset.master_tuning);
            }
            if (frame.comp_trigger) {
                dsp_note_on_keys(&frame.comp_voicing, frame.comp_vel, g_preset.master_tuning);
                g_last_voicing = frame.comp_voicing;
            }
            if (frame.drum_mask) {
                uint8_t open_hat = (frame.drum_mask & 0x04) && (g_harmony.spice_level > 75);
                dsp_trigger_drum(frame.drum_mask, frame.drum_vel, open_hat);
            }
            harmony_tick_subdivision(&g_harmony);
        }
    }
    dsp_render_block(out, n, &g_preset);
}

static void update_screen(uint8_t force) {
    uint8_t curr_bar = g_harmony.current_bar;
    uint8_t curr_beat = g_harmony.current_beat;
    uint8_t curr_playing = g_harmony.is_playing;
    uint8_t curr_algo = g_preset.ep_tone_algo;
    uint8_t curr_key = g_harmony.root_semitone;

    if (!force && curr_bar == g_last_bar && curr_beat == g_last_beat &&
        curr_playing == g_prev_playing && curr_algo == g_prev_algo && curr_key == g_prev_key) {
        return;
    }

    g_last_bar = curr_bar;
    g_last_beat = curr_beat;
    g_prev_playing = curr_playing;
    g_prev_algo = curr_algo;
    g_prev_key = curr_key;

    // Redraw dynamic areas
    lcd_fill(0, 0, 240, 240, C_BLACK);

    // Header
    draw_text_box(0, 12, 240, &FONT_L, "TEXAS BLUES 1", RGB(255, 190, 0), 1);
    draw_text_box(0, 46, 240, &FONT_S, "Hardware Checkpoint 2", RGB(150, 150, 150), 1);

    // Active Chord
    BarSlot slot = harmony_get_current_bar_slot(&g_harmony);
    ScaleDegree deg = (curr_beat >= 2 && slot.has_split) ? slot.secondary : slot.primary;
    ScaleDegreeInfo info = harmony_get_degree_info(deg);
    uint8_t chord_root = (curr_key + info.semitone_offset) % 12;
    const char *q_str = "7";
    if (info.quality == CHORD_MIN7) q_str = "m7";
    else if (info.quality == CHORD_DIM7) q_str = "dim7";

    char chord_buf[16] = {0};
    int ci = 0;
    const char *rn = NOTE_NAMES[chord_root];
    while (*rn && ci < 14) chord_buf[ci++] = *rn++;
    while (*q_str && ci < 15) chord_buf[ci++] = *q_str++;
    chord_buf[ci] = '\0';

    draw_text_box(0, 80, 240, &FONT_L, chord_buf, RGB(255, 255, 255), 1);

    // Bar and Beat
    char bar_buf[32] = {0};
    bar_buf[0] = 'B'; bar_buf[1] = 'a'; bar_buf[2] = 'r'; bar_buf[3] = ' ';
    int bnum = curr_bar + 1;
    if (bnum >= 10) {
        bar_buf[4] = '1';
        bar_buf[5] = (char)('0' + (bnum - 10));
        bar_buf[6] = ' '; bar_buf[7] = '/'; bar_buf[8] = ' '; bar_buf[9] = '1'; bar_buf[10] = '2';
        ci = 11;
    } else {
        bar_buf[4] = (char)('0' + bnum);
        bar_buf[5] = ' '; bar_buf[6] = '/'; bar_buf[7] = ' '; bar_buf[8] = '1'; bar_buf[9] = '2';
        ci = 10;
    }
    bar_buf[ci++] = ' '; bar_buf[ci++] = ' ';
    bar_buf[ci++] = 'B'; bar_buf[ci++] = 'e'; bar_buf[ci++] = 'a'; bar_buf[ci++] = 't'; bar_buf[ci++] = ' ';
    bar_buf[ci++] = (char)('1' + curr_beat);
    bar_buf[ci] = '\0';
    draw_text_box(0, 122, 240, &FONT_S, bar_buf, RGB(100, 220, 255), 1);

    // Groove Style & Tempo
    char tempo_buf[32] = {0};
    const char *t_prefix = "Shuffle ";
    ci = 0;
    while (*t_prefix) tempo_buf[ci++] = *t_prefix++;
    uint32_t bpm = g_preset.bpm;
    if (bpm >= 100) tempo_buf[ci++] = (char)('0' + (bpm / 100));
    tempo_buf[ci++] = (char)('0' + ((bpm / 10) % 10));
    tempo_buf[ci++] = (char)('0' + (bpm % 10));
    tempo_buf[ci++] = ' '; tempo_buf[ci++] = 'B'; tempo_buf[ci++] = 'P'; tempo_buf[ci++] = 'M';
    tempo_buf[ci] = '\0';
    draw_text_box(0, 148, 240, &FONT_S, tempo_buf, RGB(200, 200, 200), 1);

    // Instrument Tone
    char tone_buf[32] = {0};
    const char *algo_str = ALGO_NAMES[curr_algo % 4];
    ci = 0;
    while (*algo_str && ci < 30) tone_buf[ci++] = *algo_str++;
    tone_buf[ci] = '\0';
    draw_text_box(0, 172, 240, &FONT_S, tone_buf, RGB(255, 140, 140), 1);

    // Playback Status
    if (curr_playing) {
        draw_text_box(0, 210, 240, &FONT_S, "[PLAYING] PLAY=Pause", RGB(80, 255, 80), 1);
    } else {
        draw_text_box(0, 210, 240, &FONT_S, "[PAUSED]  PLAY=Start", RGB(255, 80, 80), 1);
    }
}

static void app_main(void) {
    // Preset configuration: 112 BPM Texas Shuffle in E Major
    g_preset.magic = PRESET_MAGIC;
    g_preset.preset_id = 0;
    g_preset.root_key = 4; // E
    g_preset.groove_style = GROOVE_TEXAS_SHUFFLE;
    g_preset.bpm = 112;
    g_preset.spice = 65;
    g_preset.swing = 67;
    g_preset.ep_tone_algo = 0; // Rhodes 2-Op FM
    g_preset.bass_vol = 110;
    g_preset.keys_vol = 95;
    g_preset.drums_vol = 105;
    g_preset.reverb_mix = 35;
    g_preset.master_tuning = 0; // 440 Hz
    g_preset.bass_cutoff = 85;
    g_preset.drive_level = 20;
    g_preset.count_in_enabled = 0;
    g_preset.form_mode = FORM_12BAR;
    g_preset.opening_var = VAR_FLAT_FOUR;
    g_preset.outro_type = OUTRO_TAG;

    // Harmony initialization
    harmony_init(&g_harmony);
    g_harmony.is_playing = 1; // Auto-play immediately on power-on/flash
    g_harmony.root_semitone = 4; // E
    g_harmony.spice_level = 65;
    g_last_voicing.count = 0;

    // DSP initialization
    dsp_init();

    // Initial Screen Draw
    update_screen(1);

    uint32_t prev_notes = 0;

    for (;;) {
        uint32_t rel = 0;
        platform_poll();

        // 1. Buttons
        uint32_t b = fm1_input_edges(&rel);
        if (b & (1u << BTN_PLAY)) {
            g_harmony.is_playing = !g_harmony.is_playing;
            if (g_harmony.is_playing) {
                g_harmony.current_bar = 0;
                g_harmony.current_beat = 0;
                g_harmony.current_subdiv = 0;
                g_sample_acc = 2646000;
            }
        }
        if (b & (1u << BTN_OCT_DOWN)) {
            if (g_preset.bpm > 50) g_preset.bpm -= 4;
        }
        if (b & (1u << BTN_OCT_UP)) {
            if (g_preset.bpm < 240) g_preset.bpm += 4;
        }

        // 2. Rotary Encoders
        int32_t e;
        if ((e = fm1_enc_take(ENC_PRESETS)) != 0) {
            int32_t nk = (int32_t)g_harmony.root_semitone + e;
            while (nk < 0) nk += 12;
            g_harmony.root_semitone = (uint8_t)(nk % 12);
        }
        if ((e = fm1_enc_take(ENC_ALGORITHM)) != 0) {
            int32_t na = (int32_t)g_preset.ep_tone_algo + e;
            while (na < 0) na += 4;
            g_preset.ep_tone_algo = (uint8_t)(na % 4);
        }
        if ((e = fm1_enc_take(ENC_SELECT)) != 0) {
            int32_t nsp = (int32_t)g_harmony.spice_level + (e * 5);
            if (nsp < 0) nsp = 0;
            if (nsp > 100) nsp = 100;
            g_harmony.spice_level = (uint8_t)nsp;
        }

        // 3. Silicone Keys: change root key immediately on press
        uint32_t notes = fm1_in.notes;
        uint32_t down = notes & ~prev_notes;
        prev_notes = notes;
        for (uint32_t k = 0; k < 27u; k++) {
            if (down & (1u << k)) {
                // Map key k (0 = F3) to semitone 0..11
                uint8_t semitone = (KEY0 + k) % 12;
                g_harmony.root_semitone = semitone;
                break;
            }
        }

        // 4. Hardware LEDs
        fm1_led_key(BTN_PLAY, g_harmony.is_playing);
        fm1_led_key(BTN_REC, g_harmony.is_playing && (g_harmony.current_beat == 0));

        // 5. Screen Update (~30 Hz throttle)
        if (fm1_ms - g_last_redraw >= 33) {
            g_last_redraw = fm1_ms;
            update_screen(0);
        }
    }
}
