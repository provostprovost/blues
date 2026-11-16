#include "ui_screens.h"
#include "hal_adapter.h"
#include "blues_harmony.h"

static const char *NOTE_NAMES[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
static const char *GROOVE_NAMES[8] = { "12/8 SLOW", "TX SHUFFLE", "CHI SWING", "ROCK BLUES", "2ND LINE", "MEMPHIS SOUL", "RHUMBA BLUES", "BOOGIE WOOGIE" };
static const char *ALGO_NAMES[12]  = { "RHODES FM", "BRIGHT EP", "WURLI 200A", "B3 CLEAN", "B3 DIRTY", "CLAVINET", "ARCHTOP GTR", "BLUES HORNS", "HONKY-TONK", "TREMOLO EP", "RESONATOR", "LESLIE ORGAN" };
static const char *BASS_NAMES[4]   = { "P-BASS", "UPRIGHT", "MUTED BOOGIE", "MOOG FUNK" };
static const char *DRUM_NAMES[4]   = { "STUDIO", "BRUSH", "RETRO 808", "STAX TIGHT" };
static const char *LEAD_NAMES[8]   = { "BLUES GTR", "HARP", "TENOR SAX", "MOOG LEAD", "WARM JAZZ", "FUZZ OCTAVE", "MUTED TRPT", "PERC ORGAN" };

static uint8_t g_last_bar = 0xFF;
static uint8_t g_last_beat = 0xFF;
static uint8_t g_last_screen = 0xFF;
static uint8_t g_last_preset = 0xFF;
static uint8_t g_last_key = 0xFF;
static uint8_t g_last_algo = 0xFF;
static uint8_t g_last_bass_inst = 0xFF;
static uint8_t g_last_drum = 0xFF;
static uint8_t g_last_lead = 0xFF;
static uint8_t g_last_playing = 0xFF;
static uint8_t g_last_groove = 0xFF;
static uint8_t g_last_spice = 0xFF;
static uint8_t g_last_bpm = 0xFF;

static uint8_t g_last_m_bass_vol = 0xFF;
static uint8_t g_last_m_keys_vol = 0xFF;
static uint8_t g_last_m_drums_vol = 0xFF;
static uint8_t g_last_m_lead_vol = 0xFF;
static uint8_t g_last_m_reverb = 0xFF;
static uint8_t g_last_m_spice = 0xFF;
static uint8_t g_last_m_swing = 0xFF;

void ui_init(void) {
    g_last_bar = 0xFF;
    g_last_beat = 0xFF;
    g_last_screen = 0xFF;
    g_last_preset = 0xFF;
    g_last_key = 0xFF;
    g_last_algo = 0xFF;
    g_last_bass_inst = 0xFF;
    g_last_drum = 0xFF;
    g_last_lead = 0xFF;
    g_last_playing = 0xFF;
    g_last_groove = 0xFF;
    g_last_spice = 0xFF;
    g_last_bpm = 0xFF;

    g_last_m_bass_vol = 0xFF;
    g_last_m_keys_vol = 0xFF;
    g_last_m_drums_vol = 0xFF;
    g_last_m_lead_vol = 0xFF;
    g_last_m_reverb = 0xFF;
    g_last_m_spice = 0xFF;
    g_last_m_swing = 0xFF;
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

static void render_single_bar_box(const HarmonyState *hs, uint8_t b, uint8_t is_current) {
    uint8_t total_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
    if (b >= total_bars) return;

    uint8_t col = b % 4;
    uint8_t row = b / 4;
    int bx = 4 + col * 59;
    int by = 46 + row * 46;

    uint16_t bg_col = is_current ? RGB(235, 150, 15) : RGB(32, 32, 32);
    uint16_t fg_col = is_current ? C_BLACK : RGB(225, 225, 225);

    hal_fill_rect(bx, by, 55, 42, bg_col);

    HarmonyState tmp_hs = *hs;
    tmp_hs.current_bar = b;
    BarSlot slot = harmony_get_current_bar_slot(&tmp_hs);

    char cstr[16];
    format_chord_str(cstr, hs->root_semitone, slot.primary);
    hal_draw_text_box(bx, by + 12, 55, 0, cstr, fg_col, 1);
}

static void render_beat_pips(const HarmonyState *hs) {
    int pip_y = 190;
    for (uint8_t p = 0; p < 4; p++) {
        int px = 50 + p * 38;
        uint16_t p_col = (hs->is_playing && hs->current_beat == p) ? C_WHITE : RGB(45, 45, 45);
        hal_fill_rect(px, pip_y, 28, 12, p_col);
    }
}

static void render_header(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx) {
    (void)preset_idx;
    hal_fill_rect(0, 0, 240, 44, C_BLACK);

    // Header Title: Groove Style & Drum Kit
    char header[36];
    const char *gname = GROOVE_NAMES[preset->groove_style % 8];
    const char *dname = DRUM_NAMES[preset->drum_kit % 4];
    int hlen = 0;
    while (*gname && hlen < 16) header[hlen++] = *gname++;
    header[hlen++] = ' ';
    header[hlen++] = '[';
    while (*dname && hlen < 32) header[hlen++] = *dname++;
    header[hlen++] = ']';
    header[hlen] = '\0';
    hal_draw_text_box(0, 6, 240, 0, header, RGB(255, 190, 0), 1);

    // Subheader: Key, BPM, Spice %
    char subheader[32];
    const char *kname = NOTE_NAMES[hs->root_semitone % 12];
    int slen = 0;
    subheader[slen++] = 'K'; subheader[slen++] = 'E'; subheader[slen++] = 'Y'; subheader[slen++] = ':';
    while (*kname && slen < 8) subheader[slen++] = *kname++;
    subheader[slen++] = ' ';
    subheader[slen++] = 'B'; subheader[slen++] = 'P'; subheader[slen++] = 'M'; subheader[slen++] = ':';
    uint8_t bpm = preset->bpm;
    if (bpm >= 100) subheader[slen++] = '0' + (bpm / 100);
    subheader[slen++] = '0' + ((bpm % 100) / 10);
    subheader[slen++] = '0' + (bpm % 10);
    subheader[slen++] = ' ';
    subheader[slen++] = 'S'; subheader[slen++] = 'P'; subheader[slen++] = 'C'; subheader[slen++] = ':';
    uint8_t spc = hs->spice_level;
    if (spc >= 100) subheader[slen++] = '1';
    subheader[slen++] = '0' + ((spc % 100) / 10);
    subheader[slen++] = '0' + (spc % 10);
    subheader[slen++] = '%';
    subheader[slen] = '\0';
    hal_draw_text_box(0, 24, 240, 0, subheader, RGB(180, 180, 180), 1);
}

static void render_status_and_footer(const HarmonyState *hs, const PresetConfig *preset) {
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

    char footer[40];
    const char *lname = LEAD_NAMES[preset->lead_instrument % 8];
    const char *aname = ALGO_NAMES[preset->ep_tone_algo % 12];
    int flen = 0;
    while (*lname && flen < 20) footer[flen++] = *lname++;
    footer[flen++] = ' ';
    footer[flen++] = '*';
    footer[flen++] = ' ';
    while (*aname && flen < 38) footer[flen++] = *aname++;
    footer[flen] = '\0';
    hal_fill_rect(0, 224, 240, 16, C_BLACK);
    hal_draw_text_box(0, 224, 240, 0, footer, RGB(140, 140, 140), 1);
}

static void render_meter(int y, const char *label, uint8_t val, uint8_t max_val, uint16_t color, uint8_t full_redraw, uint8_t *last_val) {
    if (full_redraw) {
        hal_draw_text_box(10, y, 85, 0, label, RGB(200, 200, 200), 0);
        hal_fill_rect(98, y + 2, 132, 10, RGB(35, 35, 35));
        int fill_w = ((int)val * 132) / max_val;
        if (fill_w > 132) fill_w = 132;
        if (fill_w > 0) {
            hal_fill_rect(98, y + 2, fill_w, 10, color);
        }
        *last_val = val;
    } else if (val != *last_val) {
        hal_fill_rect(98, y + 2, 132, 10, RGB(35, 35, 35));
        int fill_w = ((int)val * 132) / max_val;
        if (fill_w > 132) fill_w = 132;
        if (fill_w > 0) {
            hal_fill_rect(98, y + 2, fill_w, 10, color);
        }
        *last_val = val;
    }
}

static void render_mixer_view(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t full_redraw) {
    (void)preset_idx;
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);
        hal_draw_text_box(0, 6, 240, 0, "TB-1 BAND MIXER", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BAS K2:KEY K3:DRM K4:LD", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: REV/SPC/SWG/TUNE", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: LEAD SHEET VIEW", RGB(140, 140, 140), 1);
    }

    uint8_t l_vol = preset->lead_vol > 0 ? preset->lead_vol : 115;
    render_meter(26,  "BASS VOL",  preset->bass_vol,  127, RGB(80, 160, 255), full_redraw, &g_last_m_bass_vol);
    render_meter(48,  "KEYS VOL",  preset->keys_vol,  127, RGB(255, 180, 40), full_redraw, &g_last_m_keys_vol);
    render_meter(70,  "DRUMS VOL", preset->drums_vol, 127, RGB(255, 80, 80), full_redraw, &g_last_m_drums_vol);
    render_meter(92,  "LEAD VOL",  l_vol,             127, RGB(240, 120, 255), full_redraw, &g_last_m_lead_vol);
    render_meter(114, "REVERB",    preset->reverb_mix,127, RGB(180, 100, 255), full_redraw, &g_last_m_reverb);
    render_meter(136, "SPICE",     hs->spice_level,   100, RGB(255, 220, 50), full_redraw, &g_last_m_spice);
    render_meter(158, "SWING",     preset->swing,     100, RGB(80, 240, 120), full_redraw, &g_last_m_swing);
}

void ui_render(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t screen_id, uint8_t force_redraw) {
    uint8_t screen_switched = (screen_id != g_last_screen);
    uint8_t header_changed = screen_switched || (preset_idx != g_last_preset) ||
                             (preset->groove_style != g_last_groove) || (preset->drum_kit != g_last_drum) ||
                             (hs->root_semitone != g_last_key) || (preset->bpm != g_last_bpm) ||
                             (hs->spice_level != g_last_spice) || force_redraw;

    uint8_t full_grid_needed = screen_switched || (preset_idx != g_last_preset) ||
                              (hs->root_semitone != g_last_key) || (hs->form_mode != preset->form_mode) || force_redraw;

    if (screen_id == 0) {
        if (screen_switched) {
            hal_fill_rect(0, 0, 240, 240, C_BLACK);
        }

        if (header_changed) {
            render_header(hs, preset, preset_idx);
        }

        if (full_grid_needed) {
            uint8_t total_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
            for (uint8_t b = 0; b < total_bars; b++) {
                render_single_bar_box(hs, b, (b == hs->current_bar));
            }
        } else if (hs->current_bar != g_last_bar) {
            // Differential update: ONLY redraw previous and new active bar boxes!
            if (g_last_bar < 12) {
                render_single_bar_box(hs, g_last_bar, 0);
            }
            render_single_bar_box(hs, hs->current_bar, 1);
        }

        if (screen_switched || hs->current_beat != g_last_beat || full_grid_needed) {
            render_beat_pips(hs);
        }

        uint8_t footer_changed = screen_switched || (hs->is_playing != g_last_playing) ||
                                 (preset->ep_tone_algo != g_last_algo) || (preset->lead_instrument != g_last_lead) ||
                                 (hs->stop_time_counter != 0) || (hs->is_vamp_mode != 0) || (hs->ending_armed != 0);

        if (footer_changed || force_redraw) {
            render_status_and_footer(hs, preset);
        }
    } else {
        render_mixer_view(hs, preset, preset_idx, screen_switched || force_redraw);
    }

    g_last_bar = hs->current_bar;
    g_last_beat = hs->current_beat;
    g_last_screen = screen_id;
    g_last_preset = preset_idx;
    g_last_key = hs->root_semitone;
    g_last_algo = preset->ep_tone_algo;
    g_last_bass_inst = preset->bass_instrument;
    g_last_drum = preset->drum_kit;
    g_last_lead = preset->lead_instrument;
    g_last_playing = hs->is_playing;
    g_last_groove = preset->groove_style;
    g_last_spice = hs->spice_level;
    g_last_bpm = preset->bpm;
}
