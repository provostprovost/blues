#include "ui_screens.h"
#include "hal_adapter.h"
#include "blues_harmony.h"

static const char *NOTE_NAMES[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
static const char *GROOVE_NAMES[5] = { "12/8 SLOW", "TX SHUFFLE", "CHI SWING", "ROCK BLUES", "2ND LINE" };
static const char *ALGO_NAMES[4]   = { "RHODES FM", "BRIGHT EP", "B3 ORGAN", "CLAVINET" };

static uint8_t g_last_bar = 0xFF;
static uint8_t g_last_beat = 0xFF;
static uint8_t g_last_screen = 0xFF;
static uint8_t g_last_preset = 0xFF;
static uint8_t g_last_key = 0xFF;
static uint8_t g_last_algo = 0xFF;
static uint8_t g_last_playing = 0xFF;
static uint8_t g_last_groove = 0xFF;
static uint8_t g_last_spice = 0xFF;

void ui_init(void) {
    g_last_bar = 0xFF;
    g_last_beat = 0xFF;
    g_last_screen = 0xFF;
    g_last_preset = 0xFF;
    g_last_key = 0xFF;
    g_last_algo = 0xFF;
    g_last_playing = 0xFF;
    g_last_groove = 0xFF;
    g_last_spice = 0xFF;
}

static void format_chord_str(char *buf, uint8_t root_key, ScaleDegree deg) {
    ScaleDegreeInfo info = harmony_get_degree_info(deg);
    uint8_t chord_root = (root_key + info.semitone_offset) % 12;
    const char *rn = NOTE_NAMES[chord_root];
    const char *q_str = "7";
    if (info.quality == CHORD_MIN7) q_str = "m7";
    else if (info.quality == CHORD_DIM7) q_str = "dim7";

    int ci = 0;
    while (*rn && ci < 6) buf[ci++] = *rn++;
    while (*q_str && ci < 9) buf[ci++] = *q_str++;
    buf[ci] = '\0';
}

static void render_lead_sheet(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t full_redraw) {
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);

        // Header Title: Preset and Groove
        char header[32];
        const char *gname = GROOVE_NAMES[preset->groove_style % 5];
        // Format: P01: TX SHUFFLE
        int hlen = 0;
        header[hlen++] = 'P';
        header[hlen++] = '0' + ((preset_idx + 1) / 10);
        header[hlen++] = '0' + ((preset_idx + 1) % 10);
        header[hlen++] = ':';
        header[hlen++] = ' ';
        while (*gname && hlen < 28) header[hlen++] = *gname++;
        header[hlen] = '\0';
        hal_draw_text_box(0, 6, 240, 0, header, RGB(255, 190, 0), 1);

        // Subheader: Key, BPM, Form
        char subheader[32];
        const char *kname = NOTE_NAMES[hs->root_semitone % 12];
        int slen = 0;
        subheader[slen++] = 'K'; subheader[slen++] = 'E'; subheader[slen++] = 'Y'; subheader[slen++] = ':'; subheader[slen++] = ' ';
        while (*kname && slen < 12) subheader[slen++] = *kname++;
        subheader[slen++] = ' '; subheader[slen++] = ' ';
        subheader[slen++] = 'B'; subheader[slen++] = 'P'; subheader[slen++] = 'M'; subheader[slen++] = ':';
        uint8_t bpm = preset->bpm;
        if (bpm >= 100) subheader[slen++] = '0' + (bpm / 100);
        subheader[slen++] = '0' + ((bpm % 100) / 10);
        subheader[slen++] = '0' + (bpm % 10);
        subheader[slen++] = ' '; subheader[slen++] = ' ';
        if (hs->form_mode == FORM_MINOR_BLUES) {
            subheader[slen++] = 'M'; subheader[slen++] = 'I'; subheader[slen++] = 'N'; subheader[slen++] = 'O'; subheader[slen++] = 'R';
        } else {
            subheader[slen++] = '1'; subheader[slen++] = '2'; subheader[slen++] = 'B'; subheader[slen++] = 'A'; subheader[slen++] = 'R';
        }
        subheader[slen] = '\0';
        hal_draw_text_box(0, 24, 240, 0, subheader, RGB(180, 180, 180), 1);
    }

    // 12-Measure Grid (3 rows x 4 columns)
    // Box dimensions: w = 54, h = 42. Spacing: dx = 59, dy = 46.
    uint8_t total_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
    for (uint8_t b = 0; b < 12; b++) {
        if (b >= total_bars) {
            continue;
        }
        uint8_t col = b % 4;
        uint8_t row = b / 4;
        int bx = 4 + col * 59;
        int by = 46 + row * 46;

        uint8_t is_current = (b == hs->current_bar);
        uint16_t bg_col = is_current ? RGB(235, 150, 15) : RGB(32, 32, 32);
        uint16_t fg_col = is_current ? C_BLACK : RGB(225, 225, 225);

        hal_fill_rect(bx, by, 55, 42, bg_col);

        // Get Chord for this bar
        HarmonyState tmp_hs = *hs;
        tmp_hs.current_bar = b;
        BarSlot slot = harmony_get_current_bar_slot(&tmp_hs);

        char cstr[16];
        format_chord_str(cstr, hs->root_semitone, slot.primary);
        hal_draw_text_box(bx, by + 12, 55, 0, cstr, fg_col, 1);
    }

    // Bottom Beat Indicator (4 pips: beats 1..4)
    int pip_y = 190;
    for (uint8_t p = 0; p < 4; p++) {
        int px = 50 + p * 38;
        uint16_t p_col = (hs->is_playing && hs->current_beat == p) ? C_WHITE : RGB(45, 45, 45);
        hal_fill_rect(px, pip_y, 28, 12, p_col);
    }

    // Status Banner
    const char *status_str = "STOPPED";
    uint16_t status_col = RGB(160, 160, 160);
    if (hs->is_playing) {
        if (hs->stop_time_counter > 0) {
            status_str = "STOP-TIME BREAK";
            status_col = RGB(255, 80, 80);
        } else if (hs->is_vamp_mode) {
            status_str = "VAMP LOOP ACTIVE";
            status_col = RGB(80, 200, 255);
        } else if (hs->ending_armed) {
            status_str = "ENDING ARMED";
            status_col = RGB(255, 100, 100);
        } else if (hs->has_queued_key) {
            status_str = "KEY MOD QUEUED";
            status_col = RGB(255, 220, 80);
        } else {
            status_str = "PLAYING";
            status_col = RGB(80, 240, 80);
        }
    }
    hal_fill_rect(0, 208, 240, 16, C_BLACK);
    hal_draw_text_box(0, 208, 240, 0, status_str, status_col, 1);

    // Bottom Algorithm Strip
    char footer[32];
    const char *aname = ALGO_NAMES[preset->ep_tone_algo % 4];
    int flen = 0;
    while (*aname && flen < 20) footer[flen++] = *aname++;
    footer[flen] = '\0';
    hal_fill_rect(0, 224, 240, 16, C_BLACK);
    hal_draw_text_box(0, 224, 240, 0, footer, RGB(140, 140, 140), 1);
}

static void render_meter(int y, const char *label, uint8_t val, uint8_t max_val, uint16_t color) {
    hal_draw_text_box(10, y, 90, 0, label, RGB(200, 200, 200), 0);
    hal_fill_rect(100, y + 2, 128, 10, RGB(35, 35, 35));
    int fill_w = ((int)val * 128) / max_val;
    if (fill_w > 128) fill_w = 128;
    if (fill_w > 0) {
        hal_fill_rect(100, y + 2, fill_w, 10, color);
    }
}

static void render_mixer_view(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t full_redraw) {
    (void)preset_idx;
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);
        hal_draw_text_box(0, 8, 240, 0, "TB-1 BAND MIXER", RGB(255, 190, 0), 1);
    }

    render_meter(35,  "BASS VOL",  preset->bass_vol,  127, RGB(80, 160, 255));
    render_meter(65,  "KEYS VOL",  preset->keys_vol,  127, RGB(255, 180, 40));
    render_meter(95,  "DRUMS VOL", preset->drums_vol, 127, RGB(255, 80, 80));
    render_meter(125, "REVERB",    preset->reverb_mix,127, RGB(180, 100, 255));
    render_meter(155, "SPICE",     hs->spice_level,   100, RGB(255, 220, 50));
    render_meter(185, "SWING",     preset->swing,     100, RGB(80, 240, 120));

    hal_fill_rect(0, 218, 240, 18, C_BLACK);
    hal_draw_text_box(0, 218, 240, 0, "PRESS HOME: LEAD SHEET", RGB(140, 140, 140), 1);
}

void ui_render(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t screen_id, uint8_t force_redraw) {
    uint8_t full = force_redraw || (screen_id != g_last_screen) ||
                   (preset_idx != g_last_preset) || (preset->groove_style != g_last_groove) ||
                   (hs->root_semitone != g_last_key) || (preset->ep_tone_algo != g_last_algo);

    if (!full && hs->current_bar == g_last_bar && hs->current_beat == g_last_beat &&
        hs->is_playing == g_last_playing && hs->spice_level == g_last_spice) {
        return;
    }

    g_last_bar = hs->current_bar;
    g_last_beat = hs->current_beat;
    g_last_screen = screen_id;
    g_last_preset = preset_idx;
    g_last_key = hs->root_semitone;
    g_last_algo = preset->ep_tone_algo;
    g_last_playing = hs->is_playing;
    g_last_groove = preset->groove_style;
    g_last_spice = hs->spice_level;

    if (screen_id == 0) {
        render_lead_sheet(hs, preset, preset_idx, full);
    } else {
        render_mixer_view(hs, preset, preset_idx, full);
    }
}
