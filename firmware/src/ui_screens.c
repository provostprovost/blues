#include "ui_screens.h"
#include "hal_adapter.h"
#include "blues_harmony.h"

static const char *NOTE_NAMES[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
static const char *GROOVE_NAMES[NUM_DRUM_PATTERNS] = {
    "12/8 SLOW", "TX SHUFFLE", "CHI SWING", "ROCK BLUES", "2ND LINE", "MEMPHIS SOUL", "RHUMBA BLUES", "BOOGIE WOOGIE",
    "PURDIE SHUF", "TRAIN BEAT", "BO DIDDLEY", "JAZZ SHUFFLE", "GOSPEL SWING", "DELTA STOMP"
};
static const char *ALGO_NAMES[NUM_KEYS_INSTRUMENTS] = {
    "RHODES FM", "BRIGHT EP", "WURLI 200A", "B3 CLEAN", "B3 DIRTY", "CLAVINET",
    "ARCHTOP GTR", "BLUES HORNS", "HONKY-TONK", "TREMOLO EP", "RESONATOR", "LESLIE ORGAN",
    "ACCORDION", "VIBRAPHONE", "FARFISA", "MELLOTRON", "CP-70 GRAND", "HARMONIUM"
};
static const char *DRUM_NAMES[NUM_DRUM_KITS] = {
    "STUDIO", "BRUSH", "RETRO 808", "STAX TIGHT", "CHI ROOM", "DRY VINT", "JAZZ CLUB", "BLUES ROCK",
    "CR-78 ANLG", "NOLA PERC", "LINN GATE", "LOFI BOX"
};
static const char *LEAD_NAMES[NUM_LEAD_INSTRUMENTS] = {
    "BLUES GTR", "HARP", "TENOR SAX", "MOOG LEAD", "WARM JAZZ", "FUZZ OCTAVE", "MUTED TRPT", "PERC ORGAN",
    "TALK-BOX", "SLIDE GTR", "WAVEFOLDER", "HARD SYNC", "SID CHIP", "STEELPAN"
};
static const char *LEAD_PLAY_BADGES[NUM_LEAD_PLAY_STYLES] = { "", " [GLD]", " [BND]", " [OCT]" };
static const char *BASS_NAMES[NUM_BASS_INSTRUMENTS] = {
    "P-BASS", "UPRIGHT", "MUTED BOOGIE", "MOOG FUNK", "60S JAZZ", "FLATWOUND", "AMPEG TUBE", "SUB OCTAVE",
    "LATELY FM", "SLAP POP", "HOFNER", "ACID 303", "TUBA", "PICK ROCK"
};
static const char *BASS_STYLE_NAMES[NUM_BASS_STYLES] = {
    "WALKING 4/4", "SHUFFLE", "BOOGIE BOX", "ROOT-5TH", "FUNK SYNC",
    "RUMBA", "GOSPEL WALK", "12/8 ROLL", "MOTOWN 8TH", "2ND LINE"
};
static const char *KEYS_STYLE_NAMES[NUM_KEYS_STYLES] = {
    "CHARLESTON", "BACKBEAT 2&4", "CALL & RESP", "SPARSE PAD", "CHOP 4/4",
    "STRIDE", "BOOGIE 8TH", "GOSPEL SWELL", "MONTUNO", "12/8 ARP"
};
static const char *FORM_NAMES[5]       = { "12-BAR MAJ", "QUICK CHG", "1-4-5 OPEN", "8-BAR BLUES", "MINOR BLUES" };
static const char *TUNE_NAMES[3]       = { "440 HZ", "432 HZ", "442 HZ" };

static uint8_t g_last_bar = 0xFF;
static uint8_t g_last_beat = 0xFF;
static uint8_t g_last_screen = 0xFF;
static uint8_t g_last_preset = 0xFF;
static uint8_t g_last_key = 0xFF;
static uint8_t g_last_algo = 0xFF;
static uint8_t g_last_bass_inst = 0xFF;
static uint8_t g_last_drum = 0xFF;
static uint8_t g_last_lead = 0xFF;
static uint8_t g_last_lead_play = 0xFF;
static uint8_t g_last_playing = 0xFF;
static uint8_t g_last_groove = 0xFF;
static uint8_t g_last_spice = 0xFF;
static uint8_t g_last_bpm = 0xFF;
static uint8_t g_last_form = 0xFF;
static uint8_t g_last_toast_active = 0;
static uint32_t g_last_toast_until = 0;
static uint8_t g_last_stop_time = 0xFF;
static uint8_t g_last_vamp = 0xFF;
static uint8_t g_last_ending = 0xFF;
static uint8_t g_last_queued_key = 0xFF;

static uint8_t g_last_m_bass_vol = 0xFF;
static uint8_t g_last_m_keys_vol = 0xFF;
static uint8_t g_last_m_drums_vol = 0xFF;
static uint8_t g_last_m_lead_vol = 0xFF;
static uint8_t g_last_m_reverb = 0xFF;
static uint8_t g_last_m_spice = 0xFF;
static uint8_t g_last_m_swing = 0xFF;
static uint8_t g_last_m_fullness = 0xFF;

static uint8_t g_last_s_bstyle = 0xFF;
static uint8_t g_last_s_kstyle = 0xFF;
static uint8_t g_last_s_dpat = 0xFF;
static uint8_t g_last_s_form = 0xFF;
static uint8_t g_last_s_bass = 0xFF;
static uint8_t g_last_s_keys = 0xFF;
static uint8_t g_last_s_drum = 0xFF;
static uint8_t g_last_s_tune = 0xFF;

static uint8_t g_last_f_bful = 0xFF;
static uint8_t g_last_f_kful = 0xFF;
static uint8_t g_last_f_dful = 0xFF;
static uint8_t g_last_f_mful = 0xFF;
static uint8_t g_last_f_bspc = 0xFF;
static uint8_t g_last_f_kspc = 0xFF;
static uint8_t g_last_f_dspc = 0xFF;
static uint8_t g_last_f_mspc = 0xFF;

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
    g_last_form = 0xFF;
    g_last_toast_active = 0;
    g_last_toast_until = 0;
    g_last_stop_time = 0xFF;
    g_last_vamp = 0xFF;
    g_last_ending = 0xFF;
    g_last_queued_key = 0xFF;

    g_last_m_bass_vol = 0xFF;
    g_last_m_keys_vol = 0xFF;
    g_last_m_drums_vol = 0xFF;
    g_last_m_lead_vol = 0xFF;
    g_last_m_reverb = 0xFF;
    g_last_m_spice = 0xFF;
    g_last_m_swing = 0xFF;
    g_last_m_fullness = 0xFF;
    g_last_s_bstyle = 0xFF;
    g_last_s_kstyle = 0xFF;
    g_last_s_dpat = 0xFF;
    g_last_s_form = 0xFF;
    g_last_s_bass = 0xFF;
    g_last_s_keys = 0xFF;
    g_last_s_drum = 0xFF;
    g_last_s_tune = 0xFF;
    g_last_f_bful = 0xFF;
    g_last_f_kful = 0xFF;
    g_last_f_dful = 0xFF;
    g_last_f_mful = 0xFF;
    g_last_f_bspc = 0xFF;
    g_last_f_kspc = 0xFF;
    g_last_f_dspc = 0xFF;
    g_last_f_mspc = 0xFF;
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
    const char *gname = GROOVE_NAMES[preset->groove_style % NUM_DRUM_PATTERNS];
    const char *dname = DRUM_NAMES[preset->drum_kit % NUM_DRUM_KITS];
    int hlen = 0;
    while (*gname && hlen < 16) header[hlen++] = *gname++;
    header[hlen++] = ' ';
    header[hlen++] = '[';
    while (*dname && hlen < 32) header[hlen++] = *dname++;
    header[hlen++] = ']';
    header[hlen] = '\0';
    hal_draw_text_box(0, 6, 240, 0, header, RGB(255, 190, 0), 1);

    // Subheader: Key, Form Mode, BPM, Spice %
    char subheader[32];
    const char *kname = NOTE_NAMES[hs->root_semitone % 12];
    int slen = 0;
    subheader[slen++] = 'K'; subheader[slen++] = 'E'; subheader[slen++] = 'Y'; subheader[slen++] = ':';
    while (*kname && slen < 8) subheader[slen++] = *kname++;
    if (hs->form_mode == FORM_MINOR_BLUES) {
        subheader[slen++] = 'm';
        subheader[slen++] = ' ';
        subheader[slen++] = '(';
        subheader[slen++] = 'M';
        subheader[slen++] = 'I';
        subheader[slen++] = 'N';
        subheader[slen++] = ')';
    } else if (hs->form_mode == FORM_8BAR) {
        subheader[slen++] = ' ';
        subheader[slen++] = '(';
        subheader[slen++] = '8';
        subheader[slen++] = 'B';
        subheader[slen++] = ')';
    } else if (hs->form_mode == FORM_QUICK_CHANGE) {
        subheader[slen++] = ' ';
        subheader[slen++] = '(';
        subheader[slen++] = 'Q';
        subheader[slen++] = 'C';
        subheader[slen++] = ')';
    } else {
        subheader[slen++] = ' ';
        subheader[slen++] = '(';
        subheader[slen++] = 'M';
        subheader[slen++] = 'A';
        subheader[slen++] = 'J';
        subheader[slen++] = ')';
    }
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
    uint16_t sub_col = (hs->form_mode == FORM_MINOR_BLUES) ? RGB(140, 210, 255) : RGB(180, 180, 180);
    hal_draw_text_box(0, 24, 240, 0, subheader, sub_col, 1);
}

static void render_status_and_footer(const HarmonyState *hs, const PresetConfig *preset, uint8_t toast_active) {
    const char *status_str = (hs->form_mode == FORM_MINOR_BLUES) ? "STOPPED (MINOR BLUES)" : "STOPPED";
    uint16_t status_col = RGB(160, 160, 160);
    if (toast_active && hs->toast_msg[0] != '\0') {
        status_str = hs->toast_msg;
        status_col = RGB(0, 220, 255);
    } else if (hs->is_playing) {
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
            status_str = (hs->form_mode == FORM_MINOR_BLUES) ? "PLAYING (MINOR BLUES)" : "PLAYING";
            status_col = RGB(80, 240, 80);
        }
    }
    hal_fill_rect(0, 208, 240, 16, C_BLACK);
    hal_draw_text_box(0, 208, 240, 0, status_str, status_col, 1);

    char footer[40];
    const char *lname = LEAD_NAMES[preset->lead_instrument % NUM_LEAD_INSTRUMENTS];
    const char *lbadge = LEAD_PLAY_BADGES[preset->lead_play_style % NUM_LEAD_PLAY_STYLES];
    const char *aname = ALGO_NAMES[preset->ep_tone_algo % NUM_KEYS_INSTRUMENTS];
    int flen = 0;
    while (*lname && flen < 16) footer[flen++] = *lname++;
    while (*lbadge && flen < 22) footer[flen++] = *lbadge++;
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
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: REV/SPC/SWG/FUL", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: BAND STYLE VIEW", RGB(140, 140, 140), 1);
    }

    uint8_t l_vol = preset->lead_vol > 0 ? preset->lead_vol : 115;
    render_meter(24,  "BASS VOL",  preset->bass_vol,  127, RGB(80, 160, 255), full_redraw, &g_last_m_bass_vol);
    render_meter(43,  "KEYS VOL",  preset->keys_vol,  127, RGB(255, 180, 40), full_redraw, &g_last_m_keys_vol);
    render_meter(62,  "DRUMS VOL", preset->drums_vol, 127, RGB(255, 80, 80), full_redraw, &g_last_m_drums_vol);
    render_meter(81,  "LEAD VOL",  l_vol,             127, RGB(240, 120, 255), full_redraw, &g_last_m_lead_vol);
    render_meter(100, "REVERB",    preset->reverb_mix,127, RGB(180, 100, 255), full_redraw, &g_last_m_reverb);
    render_meter(119, "SPICE",     hs->spice_level,   100, RGB(255, 220, 50), full_redraw, &g_last_m_spice);
    render_meter(138, "SWING",     preset->swing,     100, RGB(80, 240, 120), full_redraw, &g_last_m_swing);
    render_meter(157, "FULLNESS",  hs->fullness,      100, RGB(0, 220, 255), full_redraw, &g_last_m_fullness);
}

static void render_style_row(int y, const char *label, const char *val, uint16_t color, uint8_t full_redraw, uint8_t cur_idx, uint8_t *last_idx) {
    if (full_redraw) {
        hal_draw_text_box(8, y, 88, 0, label, RGB(180, 180, 180), 0);
    }
    if (full_redraw || cur_idx != *last_idx) {
        hal_fill_rect(98, y, 134, 16, RGB(24, 24, 24));
        hal_draw_text_box(102, y, 128, 0, val, color, 0);
        *last_idx = cur_idx;
    }
}

static void render_style_view(const HarmonyState *hs, const PresetConfig *preset, uint8_t full_redraw) {
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);
        hal_draw_text_box(0, 6, 240, 0, "TB-1 BAND STYLE", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BS K2:KY K3:DR K4:FRM", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: BASS / KEYS / KIT / TUNE", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: BAND FEEL VIEW", RGB(140, 140, 140), 1);
    }

    uint8_t bs = hs->bass_style % NUM_BASS_STYLES;
    uint8_t ks = hs->keys_style % NUM_KEYS_STYLES;
    uint8_t dp = hs->drum_pattern % NUM_DRUM_PATTERNS;
    uint8_t fm = (uint8_t)hs->form_mode % 5;
    uint8_t bi = preset->bass_instrument % NUM_BASS_INSTRUMENTS;
    uint8_t ki = preset->ep_tone_algo % NUM_KEYS_INSTRUMENTS;
    uint8_t di = preset->drum_kit % NUM_DRUM_KITS;
    uint8_t tn = preset->master_tuning % 3;

    render_style_row(24,  "BASS LINE", BASS_STYLE_NAMES[bs], RGB(80, 180, 255),  full_redraw, bs, &g_last_s_bstyle);
    render_style_row(43,  "KEYS RHY",  KEYS_STYLE_NAMES[ks], RGB(255, 190, 60),  full_redraw, ks, &g_last_s_kstyle);
    render_style_row(62,  "DRUM BEAT", GROOVE_NAMES[dp],     RGB(255, 95, 95),   full_redraw, dp, &g_last_s_dpat);
    render_style_row(81,  "FORM",      FORM_NAMES[fm],       RGB(80, 240, 140),  full_redraw, fm, &g_last_s_form);
    render_style_row(100, "[G] BASS",  BASS_NAMES[bi],       RGB(140, 210, 255), full_redraw, bi, &g_last_s_bass);
    render_style_row(119, "[G] KEYS",  ALGO_NAMES[ki],       RGB(255, 220, 120), full_redraw, ki, &g_last_s_keys);
    render_style_row(138, "[G] KIT",   DRUM_NAMES[di],       RGB(255, 150, 150), full_redraw, di, &g_last_s_drum);
    render_style_row(157, "[G] TUNE",  TUNE_NAMES[tn],       RGB(200, 160, 255), full_redraw, tn, &g_last_s_tune);
}

static void render_feel_view(const HarmonyState *hs, uint8_t full_redraw) {
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);
        hal_draw_text_box(0, 6, 240, 0, "TB-1 BAND FEEL", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BS K2:KY K3:DR K4:ALL", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: TRACK SPICE", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: LEAD SHEET VIEW", RGB(140, 140, 140), 1);
    }

    render_meter(24,  "BASS FUL",  hs->bass_fullness,  100, RGB(80, 160, 255),  full_redraw, &g_last_f_bful);
    render_meter(43,  "KEYS FUL",  hs->keys_fullness,  100, RGB(255, 180, 40),  full_redraw, &g_last_f_kful);
    render_meter(62,  "DRUMS FUL", hs->drums_fullness, 100, RGB(255, 80, 80),   full_redraw, &g_last_f_dful);
    render_meter(81,  "MSTR FUL",  hs->fullness,       100, RGB(0, 220, 255),   full_redraw, &g_last_f_mful);
    render_meter(100, "[G]BS SPC", hs->bass_spice,     100, RGB(140, 200, 255), full_redraw, &g_last_f_bspc);
    render_meter(119, "[G]KY SPC", hs->keys_spice,     100, RGB(255, 220, 100), full_redraw, &g_last_f_kspc);
    render_meter(138, "[G]DR SPC", hs->drums_spice,    100, RGB(255, 140, 140), full_redraw, &g_last_f_dspc);
    render_meter(157, "[G]MS SPC", hs->spice_level,    100, RGB(200, 140, 255), full_redraw, &g_last_f_mspc);
}

void ui_render(const HarmonyState *hs, const PresetConfig *preset, uint8_t preset_idx, uint8_t screen_id, uint8_t force_redraw) {
    uint8_t screen_switched = (screen_id != g_last_screen);
    uint8_t form_changed = (hs->form_mode != g_last_form);
    uint8_t header_changed = screen_switched || (preset_idx != g_last_preset) ||
                             (preset->groove_style != g_last_groove) || (preset->drum_kit != g_last_drum) ||
                             (hs->root_semitone != g_last_key) || form_changed || (preset->bpm != g_last_bpm) ||
                             (hs->spice_level != g_last_spice) || force_redraw;

    uint8_t full_grid_needed = screen_switched || (preset_idx != g_last_preset) ||
                              (hs->root_semitone != g_last_key) || form_changed || force_redraw;

    if (screen_id == 0) {
        if (screen_switched) {
            hal_fill_rect(0, 0, 240, 240, C_BLACK);
        }

        if (header_changed) {
            render_header(hs, preset, preset_idx);
        }

        if (full_grid_needed) {
            if (form_changed || screen_switched) {
                hal_fill_rect(0, 44, 240, 142, C_BLACK);
            }
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

        uint32_t now_ms = hal_millis();
        uint8_t toast_active = (hs->toast_until_ms != 0 && ((int32_t)(hs->toast_until_ms - now_ms) > 0));
        uint8_t stop_time_active = (hs->stop_time_counter > 0);
        uint8_t status_flags_changed = (stop_time_active != g_last_stop_time) ||
                                       (hs->is_vamp_mode != g_last_vamp) ||
                                       (hs->ending_armed != g_last_ending) ||
                                       (hs->has_queued_key != g_last_queued_key);
        uint8_t footer_changed = screen_switched || (hs->is_playing != g_last_playing) ||
                                 (preset->ep_tone_algo != g_last_algo) || (preset->lead_instrument != g_last_lead) ||
                                 (preset->lead_play_style != g_last_lead_play) ||
                                 form_changed || (toast_active != g_last_toast_active) ||
                                 (toast_active && hs->toast_until_ms != g_last_toast_until) ||
                                 status_flags_changed;

        if (footer_changed || force_redraw) {
            render_status_and_footer(hs, preset, toast_active);
        }
        g_last_toast_active = toast_active;
        g_last_toast_until = hs->toast_until_ms;
        g_last_stop_time = stop_time_active;
        g_last_vamp = hs->is_vamp_mode;
        g_last_ending = hs->ending_armed;
        g_last_queued_key = hs->has_queued_key;
    } else if (screen_id == 1) {
        render_mixer_view(hs, preset, preset_idx, screen_switched || force_redraw);
    } else if (screen_id == 2) {
        render_style_view(hs, preset, screen_switched || force_redraw);
    } else {
        render_feel_view(hs, screen_switched || force_redraw);
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
    g_last_lead_play = preset->lead_play_style;
    g_last_playing = hs->is_playing;
    g_last_groove = preset->groove_style;
    g_last_spice = hs->spice_level;
    g_last_bpm = preset->bpm;
    g_last_form = hs->form_mode;
}
