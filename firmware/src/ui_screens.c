#include "ui_screens.h"
#include "hal_adapter.h"
#include "blues_harmony.h"
#include "../gen/pixelart_assets.h"

static const char *NOTE_NAMES[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
static const char *GROOVE_NAMES[NUM_DRUM_PATTERNS] = {
    "12/8 SLOW", "TX SHUFFLE", "CHI SWING", "ROCK BLUES", "2ND LINE", "MEMPHIS SOUL", "RHUMBA BLUES", "BOOGIE WOOGIE",
    "PURDIE SHUF", "TRAIN BEAT", "BO DIDDLEY", "JAZZ SHUFFLE", "GOSPEL SWING", "DELTA STOMP",
    "POCKET FUNK", "NOLA FUNK", "BLUES FUNK", "SLOW NEOFUNK", "COUNTRY TWANG"
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
    "BLUES GTR", "HARP", "ALTO SAX", "TENOR SAX", "BARI SAX", "MUTED TRPT", "HORN SECT", "JAZZ FLUTE",
    "WHISTLE", "MOOG LEAD", "WARM JAZZ", "FUZZ OCTAVE", "PERC ORGAN", "ROCK B3", "TALK-BOX",
    "SLIDE GTR", "WAH GUITAR", "TELE TWANG", "FIDDLE", "BANJO", "WAVEFOLDER", "HARD SYNC",
    "SID CHIP", "STEELPAN", "THEREMIN"
};
static const char *LEAD_PLAY_BADGES[NUM_LEAD_PLAY_STYLES] = { "", " [GLD]", " [BND]", " [OCT]" };
static const char *LEAD_PLAY_NAMES[NUM_LEAD_PLAY_STYLES] = { "POLY", "LEGATO GLIDE", "BLUES BEND", "OCTAVE STACK" };
static const char *BASS_NAMES[NUM_BASS_INSTRUMENTS] = {
    "P-BASS", "UPRIGHT", "MUTED BOOGIE", "MOOG FUNK", "60S JAZZ", "FLATWOUND", "AMPEG TUBE", "SUB OCTAVE",
    "LATELY FM", "SLAP POP", "HOFNER", "ACID 303", "TUBA", "PICK ROCK"
};
static const char *BASS_STYLE_NAMES[NUM_BASS_STYLES] = {
    "WALKING 4/4", "SHUFFLE", "BOOGIE BOX", "ROOT-5TH", "FUNK SYNC",
    "RUMBA", "GOSPEL WALK", "12/8 ROLL", "MOTOWN 8TH", "2ND LINE",
    "FUNK 16TH", "COUNTRY 2"
};
static const char *KEYS_STYLE_NAMES[NUM_KEYS_STYLES] = {
    "CHARLESTON", "BACKBEAT 2&4", "CALL & RESP", "SPARSE PAD", "CHOP 4/4",
    "STRIDE", "BOOGIE 8TH", "GOSPEL SWELL", "MONTUNO", "12/8 ARP",
    "FUNK CLAV", "COUNTRY CHP"
};
static const char *FORM_NAMES[5]       = { "12-BAR MAJ", "QUICK CHG", "1-4-5 OPEN", "8-BAR BLUES", "MINOR BLUES" };
static const char *INTRO_SHORT_NAMES[NUM_INTRO_MODES] = {
    "OFF", "AUTO", "1B V7", "2B TURN", "2B FUNK", "4B BAND", "1B CLICK"
};
static const char *OUTRO_SHORT_NAMES[NUM_OUTRO_TYPES] = {
    "SLIDE", "V-bV-IV", "IV-I TRN", "1B QUICK", "STINGER"
};

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
static uint8_t g_last_easy_lead = 0xFF;
static uint8_t g_last_playing = 0xFF;
static uint8_t g_last_groove = 0xFF;
static uint8_t g_last_variation = 0xFF;
static uint8_t g_last_bpm = 0xFF;
static uint8_t g_last_form = 0xFF;
static uint8_t g_last_toast_active = 0;
static uint32_t g_last_toast_until = 0;
static uint8_t g_last_stop_time = 0xFF;
static uint8_t g_last_vamp = 0xFF;
static uint8_t g_last_vamp_bar = 0xFF;
static uint8_t g_last_queued_cue = 0xFF;
static uint8_t g_last_ending = 0xFF;
static uint8_t g_last_queued_key = 0xFF;
static uint8_t g_last_intro_mode = 0xFF;
static uint8_t g_last_outro_type = 0xFF;
static uint8_t g_last_intro_bars = 0xFF;
static uint8_t g_last_stage_strum = 0xFF;
static uint8_t g_last_stage_flash = 0xFF;
static uint8_t g_last_stage_chord = 0xFF;
static uint8_t g_last_stage_chord_flash = 0xFF;
static uint32_t g_chord_flash_until = 0;
static uint8_t g_last_stage_lead  = 0xFF;
static uint8_t g_last_stage_amp   = 0xFF;
static uint8_t g_last_heat        = 0xFF;

static uint8_t g_last_m_bass_vol = 0xFF;
static uint8_t g_last_m_keys_vol = 0xFF;
static uint8_t g_last_m_drums_vol = 0xFF;
static uint8_t g_last_m_lead_vol = 0xFF;
static uint8_t g_last_m_reverb = 0xFF;
static uint8_t g_last_m_drive = 0xFF;
static uint8_t g_last_m_swing = 0xFF;
static uint8_t g_last_m_lead_play = 0xFF;

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
    g_last_lead_play = 0xFF;
    g_last_easy_lead = 0xFF;
    g_last_playing = 0xFF;
    g_last_groove = 0xFF;
    g_last_variation = 0xFF;
    g_last_bpm = 0xFF;
    g_last_form = 0xFF;
    g_last_toast_active = 0;
    g_last_toast_until = 0;
    g_last_stop_time = 0xFF;
    g_last_vamp = 0xFF;
    g_last_vamp_bar = 0xFF;
    g_last_queued_cue = 0xFF;
    g_last_ending = 0xFF;
    g_last_queued_key = 0xFF;
    g_last_intro_mode = 0xFF;
    g_last_outro_type = 0xFF;
    g_last_intro_bars = 0xFF;
    g_last_stage_strum = 0xFF;
    g_last_stage_flash = 0xFF;
    g_last_stage_chord = 0xFF;
    g_last_stage_chord_flash = 0xFF;
    g_chord_flash_until = 0;
    g_last_stage_lead  = 0xFF;
    g_last_stage_amp   = 0xFF;
    g_last_heat        = 0xFF;

    g_last_m_bass_vol = 0xFF;
    g_last_m_keys_vol = 0xFF;
    g_last_m_drums_vol = 0xFF;
    g_last_m_lead_vol = 0xFF;
    g_last_m_reverb = 0xFF;
    g_last_m_drive = 0xFF;
    g_last_m_swing = 0xFF;
    g_last_m_lead_play = 0xFF;
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

static void draw_rect_border(int x, int y, int w, int h, int t, uint16_t col) {
    hal_fill_rect(x, y, w, t, col);
    hal_fill_rect(x, y + h - t, w, t, col);
    hal_fill_rect(x, y + t, t, h - 2 * t, col);
    hal_fill_rect(x + w - t, y + t, t, h - 2 * t, col);
}

static void render_single_bar_box(const HarmonyState *hs, uint8_t b, uint8_t is_current) {
    uint8_t total_bars = (hs->form_mode == FORM_8BAR) ? 8 : 12;
    if (b >= total_bars) return;

    uint8_t col = b % 4;
    uint8_t row = b / 4;
    int bx = 4 + col * 59;
    int by = 46 + row * 24;

    uint16_t border_col = is_current ? RGB(255, 190, 30) : RGB(58, 58, 64);
    uint16_t bg_col     = is_current ? RGB(75, 42, 0)    : RGB(24, 24, 28);
    uint16_t fg_col     = is_current ? C_WHITE           : RGB(215, 215, 220);

    draw_rect_border(bx, by, 55, 21, is_current ? 2 : 1, border_col);
    if (is_current) {
        hal_fill_rect(bx + 2, by + 2, 51, 17, bg_col);
    } else {
        hal_fill_rect(bx + 1, by + 1, 53, 19, bg_col);
    }

    HarmonyState tmp_hs = *hs;
    tmp_hs.current_bar = b;
    BarSlot slot = harmony_get_current_bar_slot(&tmp_hs);

    char cstr[16];
    format_chord_str(cstr, hs->root_semitone, slot.primary);
    hal_draw_text_box_bg(bx + 2, by + 3, 51, 0, cstr, fg_col, bg_col, 1);
}

static void draw_beamed_note_burst(int x, int y, uint16_t c1, uint16_t c2, uint8_t alt, uint8_t heat) {
    int dy = alt ? -2 : 0;
    if (heat <= 2) {
        // Level 1-2: Single quarter-note (plus 1 small spark at Level 2)
        hal_fill_rect(x + 11, y + 8 + dy, 4, 4, c1);
        hal_fill_rect(x + 13, y + 1 + dy, 2, 9, c1);
        hal_fill_rect(x + 13, y + 1 + dy, 5, 2, c1);
        if (heat == 2) {
            hal_fill_rect(x + 22, y + 4, 3, 2, c2);
        }
        return;
    }
    // Level 3-5: Beamed eighth-note pair + scaling sparks
    hal_fill_rect(x + 8,  y + 9 + dy, 4, 4, c1);
    hal_fill_rect(x + 16, y + 7 + dy, 4, 4, c1);
    hal_fill_rect(x + 10, y + 1 + dy, 2, 9, c1);
    hal_fill_rect(x + 18, y + 0 + dy, 2, 8, c1);
    hal_fill_rect(x + 10, y + 0 + dy, 10, 2, c1);
    // Radiating 8-bit sparks
    hal_fill_rect(x + (alt ? 1 : 0), y + 3, 3, 2, c2);
    hal_fill_rect(x + 24, y + (alt ? 1 : 2), 4, 2, c2);
    if (heat >= 4) {
        hal_fill_rect(x + (alt ? 5 : 4), y + 0, 2, 3, c1);
        hal_fill_rect(x + 28, y + (alt ? 4 : 5), 2, 4, c1);
        hal_fill_rect(x + 24, y + 10, 3, 2, c2);
    }
    if (heat >= 5) {
        // White-hot peak star cores
        hal_fill_rect(x + 2, y + 8, 2, 2, C_WHITE);
        hal_fill_rect(x + 29, y + 1, 2, 2, C_WHITE);
    }
}

static uint16_t get_stage_footlight_color(uint8_t heat, uint8_t on) {
    // Unified 5-Color Thermal Spectrum across all 3 band members:
    // 1=Blue, 2=Green, 3=Yellow, 4=Orange, 5=Red
    if (!on) {
        if (heat == 1) return RGB(18, 38, 85);   // Dim Blue
        if (heat == 2) return RGB(15, 65, 28);   // Dim Green
        if (heat == 3) return RGB(75, 62, 12);   // Dim Yellow
        if (heat == 4) return RGB(80, 36, 6);    // Dim Orange
        return RGB(85, 14, 14);                  // Dim Red (Heat 5)
    }
    if (heat == 1) return RGB(55, 130, 255);     // Bright Blue
    if (heat == 2) return RGB(45, 225, 95);      // Bright Green
    if (heat == 3) return RGB(255, 220, 35);     // Bright Yellow
    if (heat == 4) return RGB(255, 120, 15);     // Bright Orange
    return RGB(255, 35, 35);                     // Bright Red (Heat 5)
}

static void draw_tweed_amp_shell(int ax, uint8_t heat) {
    // 1. Brown leather handle + brass end caps
    hal_fill_rect(ax + 17, 140, 24, 4, RGB(105, 55, 22));
    hal_fill_rect(ax + 15, 142, 3, 2, RGB(225, 180, 75));
    hal_fill_rect(ax + 40, 142, 3, 2, RGB(225, 180, 75));
    // 2. Lacquered gold tweed cabinet with woven horizontal texture (h=54 -> y=144..197, leaving room for footlight at y=200)
    hal_fill_rect(ax, 144, 58, 54, RGB(215, 162, 68));
    for (int ty = 146; ty < 197; ty += 4) {
        hal_fill_rect(ax, ty, 58, 1, RGB(172, 118, 38));
    }
    draw_rect_border(ax, 144, 58, 54, 1, RGB(135, 88, 24));
    hal_fill_rect(ax, 198, 58, 2, C_BLACK);
    // Warm vacuum-tube vent glow on Heat >= 3
    if (heat >= 3) {
        uint16_t glow = (heat >= 5) ? RGB(255, 60, 30) :
                        (heat == 4) ? RGB(255, 130, 25) : RGB(255, 190, 50);
        hal_fill_rect(ax + 6, 145, 46, 2, glow);
    }
    // 3. Chrome control plate + knobs + jewel pilot lamp (matches Heat spectrum)
    hal_fill_rect(ax + 4, 147, 50, 8, RGB(195, 200, 210));
    hal_fill_rect(ax + 4, 154, 50, 1, RGB(120, 125, 135));
    hal_fill_rect(ax + 7,  149, 3, 4, RGB(30, 30, 35));
    hal_fill_rect(ax + 13, 149, 3, 4, RGB(30, 30, 35));
    hal_fill_rect(ax + 19, 149, 3, 4, RGB(30, 30, 35));
    hal_fill_rect(ax + 25, 149, 3, 4, RGB(30, 30, 35));
    hal_fill_rect(ax + 46, 149, 4, 4, get_stage_footlight_color(heat, 1));
    // 4. Oxblood/brown grill cloth with gold piping
    draw_rect_border(ax + 4, 158, 50, 36, 1, RGB(235, 185, 75));
    hal_fill_rect(ax + 5, 159, 48, 34, RGB(110, 42, 24));
    for (int gy = 161; gy < 192; gy += 3) {
        hal_fill_rect(ax + 5, gy, 48, 1, RGB(145, 65, 34));
    }
    draw_rect_border(ax + 8, 166, 42, 22, 1, RGB(220, 165, 60));
    hal_fill_rect(ax + 9, 167, 40, 20, RGB(18, 12, 10));
}

static void render_shred_stage(const HarmonyState *hs, const PresetConfig *preset, uint8_t full_redraw) {
    (void)preset;
    uint8_t heat = (hs->heat_level >= 1 && hs->heat_level <= 5) ? hs->heat_level : 3;
    uint8_t d_on = (hs->is_playing && hs->vis_drum_pulse > 0) ? 1 : 0;
    uint8_t b_on = (hs->is_playing && hs->vis_bass_pulse > 0) ? 1 : 0;
    uint8_t k_on = (hs->is_playing && hs->vis_keys_pulse > 0) ? 1 : 0;
    uint8_t alt = (hs->current_beat & 1u);

    if (full_redraw) {
        hal_fill_rect(0, 119, 240, 87, C_BLACK);
    }

    // 1. Drummer (x=4, y=138) + Unified Heat Footlight + Impact Rays
    uint8_t d_sig = (uint8_t)((heat << 4) | (d_on << 1) | alt);
    if (full_redraw || d_sig != g_last_stage_flash) {
        const uint8_t *dspr = !d_on ? DRUM_SPRITE_UP : (heat <= 2) ? DRUM_SPRITE_SOFT : DRUM_SPRITE_DOWN;
        hal_blit_sprite_2x(4, 138, DRUM_SPRITE_W, DRUM_SPRITE_H, DRUM_SPRITE_PAL, dspr);
        hal_fill_rect(6, 200, 50, 3, get_stage_footlight_color(heat, d_on));
        hal_fill_rect(0, 126, 60, 10, C_BLACK);
        if (d_on) {
            uint16_t r_hi = get_stage_footlight_color(heat, 1);
            uint16_t r_lo = (heat >= 5) ? C_WHITE : RGB(255, 210, 120);
            hal_fill_rect(12, 132, 4, 2, r_hi); hal_fill_rect(42, 132, 4, 2, r_hi);
            if (heat >= 2) {
                hal_fill_rect(18, 129, 2, 3, r_lo); hal_fill_rect(38, 129, 2, 3, r_lo);
            }
            if (heat >= 4) {
                hal_fill_rect(8,  134, 3, 2, r_lo); hal_fill_rect(47, 134, 3, 2, r_lo);
            }
        }
        g_last_stage_flash = d_sig;
    }

    // 2. Guitarist/Bassist (x=64, y=138) + Unified Heat Footlight + Notes & Sparks
    uint8_t b_sig = (uint8_t)((heat << 4) | (b_on << 1) | alt);
    if (full_redraw || b_sig != g_last_stage_strum) {
        const uint8_t *bspr = !b_on ? SRV_SPRITE_UP : (heat <= 2) ? SRV_SPRITE_SOFT : SRV_SPRITE_DOWN;
        hal_blit_sprite_2x(64, 138, SRV_SPRITE_W, SRV_SPRITE_H, SRV_SPRITE_PAL, bspr);
        hal_fill_rect(66, 200, 44, 3, get_stage_footlight_color(heat, b_on));
        hal_fill_rect(64, 119, 52, 17, C_BLACK);
        if (b_on) {
            draw_beamed_note_burst(74, 122, RGB(255, 215, 60), RGB(255, 150, 40), alt, heat);
        }
        g_last_stage_strum = b_sig;
    }

    // 3. Cool Girl Keyboardist (x=118, y=138) + Unified Heat Footlight + Notes & Sparks
    uint8_t k_sig = (uint8_t)((heat << 4) | (k_on << 1) | alt);
    if (full_redraw || k_sig != g_last_beat) {
        const uint8_t *kspr = !k_on ? KEYS_SPRITE_UP : (heat <= 2) ? KEYS_SPRITE_SOFT : KEYS_SPRITE_DOWN;
        hal_blit_sprite_2x(118, 138, KEYS_SPRITE_W, KEYS_SPRITE_H, KEYS_SPRITE_PAL, kspr);
        hal_fill_rect(120, 200, 50, 3, get_stage_footlight_color(heat, k_on));
        hal_fill_rect(120, 119, 54, 17, C_BLACK);
        if (k_on) {
            draw_beamed_note_burst(130, 122, RGB(60, 225, 255), RGB(180, 250, 255), alt ^ 1u, heat);
        }
        g_last_beat = k_sig;
    }

    // 4. Fender Blues Junior Mini-Amp (Redrawn only when Heat changes — zero flicker) + Live Chord
    uint8_t amp_redrawn = 0;
    if (full_redraw || heat != g_last_stage_amp) {
        draw_tweed_amp_shell(178, heat);
        g_last_stage_amp = heat;
        amp_redrawn = 1;
    }

    BarSlot slot = harmony_get_current_bar_slot(hs);
    ScaleDegree active_deg = (hs->is_playing && hs->current_beat >= 2 && slot.has_split) ? slot.secondary : slot.primary;
    uint8_t chord_sig = (uint8_t)(((hs->root_semitone & 0x0Fu) << 4) | ((uint8_t)active_deg & 0x0Fu));
    uint32_t now_ms = hal_millis();
    if (hs->is_playing && g_last_stage_chord != 0xFF && chord_sig != g_last_stage_chord) {
        g_chord_flash_until = now_ms + 240u;
    }
    uint8_t flashing = (hs->is_playing && g_chord_flash_until != 0 && ((int32_t)(g_chord_flash_until - now_ms) > 0)) ? 1 : 0;
    if (full_redraw || amp_redrawn || chord_sig != g_last_stage_chord || flashing != g_last_stage_chord_flash) {
        char cstr[16];
        format_chord_str(cstr, hs->root_semitone, active_deg);
        uint16_t border_col = RGB(220, 165, 60);
        uint16_t bg_col     = RGB(18, 12, 10);
        uint16_t fg_col     = RGB(255, 215, 70);
        if (flashing) {
            static const uint16_t HEAT_FLASH_BG[5] = {
                RGB(30, 95, 225),   // 1: Blue
                RGB(20, 155, 55),   // 2: Green
                RGB(190, 145, 0),   // 3: Yellow
                RGB(215, 85, 0),    // 4: Orange
                RGB(215, 25, 25)    // 5: Red
            };
            border_col = get_stage_footlight_color(heat, 1);
            bg_col     = HEAT_FLASH_BG[heat - 1];
            fg_col     = C_WHITE;
        }
        draw_rect_border(186, 166, 42, 22, 1, border_col);
        hal_fill_rect(187, 167, 40, 20, bg_col);
        hal_draw_text_box_bg(187, 169, 40, 0, cstr, fg_col, bg_col, 1);
        g_last_stage_chord = chord_sig;
        g_last_stage_chord_flash = flashing;
    }

    // 5. Sound waves (when lead keys held) + Amp Lead Footlight + Heat/Smoke Wisps (at Heat 5)
    uint8_t lead_on = (hal_get_keys_mask() != 0 && !(hal_get_btns_mask() & ((1u << BTN_SEL) | (1u << BTN_SAVE)))) ? 1 : 0;
    uint8_t wave_phase = (uint8_t)((hal_millis() / 90u) & 1u);
    uint8_t l_sig = (lead_on || (heat >= 5 && hs->is_playing)) ? (uint8_t)((heat << 3) | (lead_on << 1) | wave_phase) : 0u;
    if (full_redraw || amp_redrawn || l_sig != g_last_stage_lead) {
        hal_fill_rect(180, 200, 54, 3, get_stage_footlight_color(heat, lead_on));
    }
    if (full_redraw || l_sig != g_last_stage_lead) {
        hal_fill_rect(176, 120, 62, 19, C_BLACK);
        if (heat >= 5 && hs->is_playing) {
            uint16_t smk = RGB(140, 140, 155);
            int sy = wave_phase ? 0 : -2;
            hal_fill_rect(192, 134 + sy, 3, 3, smk);
            hal_fill_rect(206, 130 + sy, 3, 4, smk);
            hal_fill_rect(220, 133 + sy, 3, 3, smk);
            hal_fill_rect(196, 125 + sy, 2, 3, RGB(95, 95, 110));
            hal_fill_rect(212, 123 + sy, 2, 3, RGB(95, 95, 110));
        }
        if (lead_on) {
            uint16_t w1 = wave_phase ? RGB(255, 245, 160) : RGB(255, 195, 50);
            uint16_t w2 = wave_phase ? RGB(255, 165, 35)  : RGB(255, 245, 160);
            hal_fill_rect(198, 134, 18, 2, w1);
            hal_fill_rect(195, 132, 3, 2, w1);  hal_fill_rect(216, 132, 3, 2, w1);
            hal_fill_rect(192, 129, 20, 2, w2);
            hal_fill_rect(189, 127, 3, 2, w2);  hal_fill_rect(212, 127, 3, 2, w2);
            hal_fill_rect(186, 123, 30, 2, w1);
            hal_fill_rect(183, 121, 3, 2, w1);  hal_fill_rect(216, 121, 3, 2, w1);
            hal_fill_rect(176, 129, 4, 2, w2);  hal_fill_rect(232, 129, 4, 2, w2);
        }
        g_last_stage_lead = l_sig;
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
    subheader[slen++] = 'H'; subheader[slen++] = 'E'; subheader[slen++] = 'A'; subheader[slen++] = 'T'; subheader[slen++] = ':';
    uint8_t hl = (hs->heat_level >= 1 && hs->heat_level <= 5) ? hs->heat_level : 3;
    subheader[slen++] = '0' + hl;
    subheader[slen++] = '/';
    subheader[slen++] = '5';
    subheader[slen] = '\0';
    uint16_t sub_col = (hs->form_mode == FORM_MINOR_BLUES) ? RGB(140, 210, 255) : RGB(180, 180, 180);
    hal_draw_text_box(0, 24, 240, 0, subheader, sub_col, 1);
}

static void append_str(char *dst, int *pos, int max_len, const char *src) {
    while (*src && *pos < max_len) {
        dst[(*pos)++] = *src++;
    }
}

static void render_status_and_footer(const HarmonyState *hs, const PresetConfig *preset, uint8_t toast_active) {
    char status_buf[36];
    const char *status_str = status_buf;
    uint16_t status_col = RGB(255, 200, 60);

    uint8_t im = hs->count_in_enabled % NUM_INTRO_MODES;
    uint8_t om = (uint8_t)hs->outro_type % NUM_OUTRO_TYPES;

    if (toast_active && hs->toast_msg[0] != '\0') {
        status_str = hs->toast_msg;
        status_col = RGB(0, 220, 255);
    } else if (hs->is_playing) {
        if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) {
            int p = 0;
            append_str(status_buf, &p, 34, "OUTRO NEXT BAR: ");
            append_str(status_buf, &p, 34, OUTRO_SHORT_NAMES[om]);
            status_buf[p] = '\0';
            status_col = RGB(255, 160, 60);
        } else if (hs->ending_armed >= ENDING_BAR_1) {
            static const char *OUTRO_STATUS[NUM_OUTRO_TYPES] = {
                "ENDING: CLASSIC SLIDE", "ENDING: V-bV-IV-I", "ENDING: IV-I TURN", "ENDING: 1B QUICK", "ENDING: STINGER"
            };
            status_str = OUTRO_STATUS[om];
            status_col = RGB(255, 90, 90);
        } else if (hs->count_in_bars_remaining > 0) {
            int p = 0;
            append_str(status_buf, &p, 34, "INTRO: ");
            append_str(status_buf, &p, 34, INTRO_SHORT_NAMES[(uint8_t)hs->active_intro_style % NUM_INTRO_MODES]);
            status_buf[p] = '\0';
            status_col = RGB(0, 220, 255);
        } else if (hs->stop_time_counter > 0) {
            status_str = "STOP-TIME BREAK";
            status_col = RGB(255, 80, 80);
        } else if (hs->queued_cue == CUE_TO_VAMP || hs->queued_cue == CUE_TO_12BAR) {
            status_str = (hs->queued_cue == CUE_TO_VAMP) ? "VAMP NEXT BAR" : "12-BAR NEXT BAR";
            status_col = RGB(255, 200, 60);
        } else if (hs->is_vamp_mode) {
            char c1[12], c4[12];
            ScaleDegree d1 = (hs->form_mode == FORM_MINOR_BLUES) ? DEG_i7 : DEG_I7;
            ScaleDegree d4 = (hs->form_mode == FORM_MINOR_BLUES) ? DEG_iv7 : DEG_IV7;
            format_chord_str(c1, hs->root_semitone, d1);
            format_chord_str(c4, hs->root_semitone, d4);
            int p = 0;
            append_str(status_buf, &p, 34, "VAMP ");
            append_str(status_buf, &p, 34, c1);
            append_str(status_buf, &p, 34, "/");
            append_str(status_buf, &p, 34, c4);
            append_str(status_buf, &p, 34, " [");
            if (p < 32) status_buf[p++] = (char)('1' + (hs->vamp_bar_counter & 3u));
            append_str(status_buf, &p, 34, "/4]");
            status_buf[p] = '\0';
            status_col = RGB(80, 200, 255);
        } else if (hs->has_queued_key) {
            status_str = "KEY MOD QUEUED";
            status_col = RGB(255, 220, 80);
        } else {
            status_str = (hs->form_mode == FORM_MINOR_BLUES) ? "PLAYING (MINOR BLUES)" : "PLAYING";
            status_col = RGB(80, 240, 80);
        }
    } else {
        int p = 0;
        if (im == INTRO_OFF) {
            append_str(status_buf, &p, 34, "PRESS PLAY [OUT:");
            append_str(status_buf, &p, 34, OUTRO_SHORT_NAMES[om]);
            append_str(status_buf, &p, 34, "]");
        } else {
            append_str(status_buf, &p, 34, "PLAY [IN:");
            append_str(status_buf, &p, 34, INTRO_SHORT_NAMES[im]);
            append_str(status_buf, &p, 34, " OUT:");
            append_str(status_buf, &p, 34, OUTRO_SHORT_NAMES[om]);
            append_str(status_buf, &p, 34, "]");
        }
        status_buf[p] = '\0';
    }
    hal_fill_rect(0, 208, 240, 16, C_BLACK);
    hal_draw_text_box(0, 208, 240, 0, status_str, status_col, 1);

    char footer[40];
    const char *lname = LEAD_NAMES[preset->lead_instrument % NUM_LEAD_INSTRUMENTS];
    const char *lbadge = LEAD_PLAY_BADGES[preset->lead_play_style % NUM_LEAD_PLAY_STYLES];
    const char *ezbadge = hs->easy_lead ? " [EZ]" : "";
    const char *aname = ALGO_NAMES[preset->ep_tone_algo % NUM_KEYS_INSTRUMENTS];
    int flen = 0;
    while (*lname && flen < 16) footer[flen++] = *lname++;
    while (*lbadge && flen < 22) footer[flen++] = *lbadge++;
    while (*ezbadge && flen < 27) footer[flen++] = *ezbadge++;
    footer[flen++] = ' ';
    footer[flen++] = '*';
    footer[flen++] = ' ';
    while (*aname && flen < 38) footer[flen++] = *aname++;
    footer[flen] = '\0';
    hal_fill_rect(0, 224, 240, 16, C_BLACK);
    hal_draw_text_box(0, 224, 240, 0, footer, hs->easy_lead ? RGB(100, 235, 180) : RGB(140, 140, 140), 1);
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
        hal_draw_text_box(0, 6, 240, 0, "BLUES BOX MIXER", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BAS K2:KEY K3:DRM K4:LD", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: REV/DRV/SWG/PLAY", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "[SEL+K4/GLO+LFO]: EASY LEAD", RGB(140, 140, 140), 1);
    }

    uint8_t l_vol = preset->lead_vol;
    uint8_t lp = preset->lead_play_style % NUM_LEAD_PLAY_STYLES;
    uint8_t ez = hs->easy_lead ? 1 : 0;
    uint8_t lp_comb = (uint8_t)((ez << 4) | lp);
    render_meter(24,  "BASS VOL",  preset->bass_vol,   127, RGB(80, 160, 255), full_redraw, &g_last_m_bass_vol);
    render_meter(43,  "KEYS VOL",  preset->keys_vol,   127, RGB(255, 180, 40), full_redraw, &g_last_m_keys_vol);
    render_meter(62,  "DRUMS VOL", preset->drums_vol,  127, RGB(255, 80, 80), full_redraw, &g_last_m_drums_vol);
    render_meter(81,  "LEAD VOL",  l_vol,              127, RGB(240, 120, 255), full_redraw, &g_last_m_lead_vol);
    render_meter(100, "REVERB",    preset->reverb_mix, 127, RGB(180, 100, 255), full_redraw, &g_last_m_reverb);
    render_meter(119, "DRIVE",     preset->drive_level,127, RGB(255, 130, 50), full_redraw, &g_last_m_drive);
    render_meter(138, "SWING",     preset->swing,      100, RGB(80, 240, 120), full_redraw, &g_last_m_swing);
    if (full_redraw) hal_draw_text_box(10, 157, 85, 0, "LEAD PLAY", RGB(200, 200, 200), 0);
    if (full_redraw || lp_comb != g_last_m_lead_play) {
        static const char *LP_EZ_NAMES[NUM_LEAD_PLAY_STYLES] = {
            "POLY [EASY]", "GLIDE [EASY]", "BEND [EASY]", "OCTAVE [EASY]"
        };
        const char *lp_str = ez ? LP_EZ_NAMES[lp] : LEAD_PLAY_NAMES[lp];
        hal_fill_rect(100, 157, 130, 16, RGB(24, 24, 24));
        hal_draw_text_box(104, 157, 124, 0, lp_str, ez ? RGB(100, 255, 180) : RGB(240, 160, 255), 0);
        g_last_m_lead_play = lp_comb;
    }
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
        hal_draw_text_box(0, 6, 240, 0, "BLUES BOX STYLE", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BS K2:KY K3:DR K4:FRM", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: BASS / KIT / INTRO / OUT", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: BAND FEEL VIEW", RGB(140, 140, 140), 1);
    }

    uint8_t bs = hs->bass_style % NUM_BASS_STYLES;
    uint8_t ks = hs->keys_style % NUM_KEYS_STYLES;
    uint8_t dp = hs->drum_pattern % NUM_DRUM_PATTERNS;
    uint8_t fm = (uint8_t)hs->form_mode % 5;
    uint8_t bi = preset->bass_instrument % NUM_BASS_INSTRUMENTS;
    uint8_t di = preset->drum_kit % NUM_DRUM_KITS;
    uint8_t im = hs->count_in_enabled % NUM_INTRO_MODES;
    uint8_t om = (uint8_t)hs->outro_type % NUM_OUTRO_TYPES;

    render_style_row(24,  "BASS LINE", BASS_STYLE_NAMES[bs], RGB(80, 180, 255),  full_redraw, bs, &g_last_s_bstyle);
    render_style_row(43,  "KEYS RHY",  KEYS_STYLE_NAMES[ks], RGB(255, 190, 60),  full_redraw, ks, &g_last_s_kstyle);
    render_style_row(62,  "DRUM BEAT", GROOVE_NAMES[dp],     RGB(255, 95, 95),   full_redraw, dp, &g_last_s_dpat);
    render_style_row(81,  "FORM",      FORM_NAMES[fm],       RGB(80, 240, 140),  full_redraw, fm, &g_last_s_form);
    render_style_row(100, "[G] BASS",  BASS_NAMES[bi],       RGB(140, 210, 255), full_redraw, bi, &g_last_s_bass);
    render_style_row(119, "[G] KIT",   DRUM_NAMES[di],       RGB(255, 150, 150), full_redraw, di, &g_last_s_drum);
    render_style_row(138, "[G] INTRO", INTRO_SHORT_NAMES[im],RGB(0, 220, 255),   full_redraw, im, &g_last_s_keys);
    render_style_row(157, "[G] OUTRO", OUTRO_SHORT_NAMES[om],RGB(255, 140, 100), full_redraw, om, &g_last_s_tune);
}

static void render_feel_view(const HarmonyState *hs, uint8_t full_redraw) {
    if (full_redraw) {
        hal_fill_rect(0, 0, 240, 240, C_BLACK);
        hal_draw_text_box(0, 6, 240, 0, "BLUES BOX FEEL", RGB(255, 190, 0), 1);
        hal_draw_text_box(0, 180, 240, 0, "K1:BS K2:KY K3:DR K4:ALL", RGB(0, 220, 255), 1);
        hal_draw_text_box(0, 198, 240, 0, "[GLO]: TRACK VARIATION", RGB(255, 180, 0), 1);
        hal_draw_text_box(0, 216, 240, 0, "HOME: LEAD SHEET VIEW", RGB(140, 140, 140), 1);
    }

    render_meter(24,  "BASS FUL",  hs->bass_fullness,  100, RGB(80, 160, 255),  full_redraw, &g_last_f_bful);
    render_meter(43,  "KEYS FUL",  hs->keys_fullness,  100, RGB(255, 180, 40),  full_redraw, &g_last_f_kful);
    render_meter(62,  "DRUMS FUL", hs->drums_fullness, 100, RGB(255, 80, 80),   full_redraw, &g_last_f_dful);
    render_meter(81,  "MSTR FUL",  hs->fullness,       100, RGB(0, 220, 255),   full_redraw, &g_last_f_mful);
    render_meter(100, "[G]BS VAR", hs->bass_variation, 100, RGB(140, 200, 255), full_redraw, &g_last_f_bspc);
    render_meter(119, "[G]KY VAR", hs->keys_variation, 100, RGB(255, 220, 100), full_redraw, &g_last_f_kspc);
    render_meter(138, "[G]DR VAR", hs->drums_variation,100, RGB(255, 140, 140), full_redraw, &g_last_f_dspc);
    render_meter(157, "[G]MS VAR", hs->variation,      100, RGB(200, 140, 255), full_redraw, &g_last_f_mspc);
}

void ui_render(const HarmonyState *hs_live, const PresetConfig *preset, uint8_t preset_idx, uint8_t screen_id, uint8_t force_redraw) {
    HarmonyState snap = *hs_live;
    const HarmonyState *hs = &snap;
    uint8_t screen_switched = (screen_id != g_last_screen);
    uint8_t form_changed = (hs->form_mode != g_last_form);
    uint8_t header_changed = screen_switched || (preset_idx != g_last_preset) ||
                             (preset->groove_style != g_last_groove) || (preset->drum_kit != g_last_drum) ||
                             (hs->root_semitone != g_last_key) || form_changed || (preset->bpm != g_last_bpm) ||
                             (hs->variation != g_last_variation) || (hs->heat_level != g_last_heat) || force_redraw;

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
                hal_fill_rect(0, 44, 240, 76, C_BLACK);
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

        render_shred_stage(hs, preset, screen_switched || force_redraw);

        uint32_t now_ms = hal_millis();
        uint8_t toast_active = (hs->toast_until_ms != 0 && ((int32_t)(hs->toast_until_ms - now_ms) > 0));
        uint8_t stop_time_active = (hs->stop_time_counter > 0);
        uint8_t status_flags_changed = (stop_time_active != g_last_stop_time) ||
                                       (hs->is_vamp_mode != g_last_vamp) ||
                                       (hs->vamp_bar_counter != g_last_vamp_bar) ||
                                       ((uint8_t)hs->queued_cue != g_last_queued_cue) ||
                                       (hs->ending_armed != g_last_ending) ||
                                       (hs->has_queued_key != g_last_queued_key) ||
                                       (hs->count_in_enabled != g_last_intro_mode) ||
                                       ((uint8_t)hs->outro_type != g_last_outro_type) ||
                                       (hs->count_in_bars_remaining != g_last_intro_bars);
        uint8_t footer_changed = screen_switched || (hs->is_playing != g_last_playing) ||
                                 (preset->ep_tone_algo != g_last_algo) || (preset->lead_instrument != g_last_lead) ||
                                 (preset->lead_play_style != g_last_lead_play) || (hs->easy_lead != g_last_easy_lead) ||
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
        g_last_vamp_bar = hs->vamp_bar_counter;
        g_last_queued_cue = (uint8_t)hs->queued_cue;
        g_last_ending = hs->ending_armed;
        g_last_queued_key = hs->has_queued_key;
        g_last_intro_mode = hs->count_in_enabled;
        g_last_outro_type = (uint8_t)hs->outro_type;
        g_last_intro_bars = hs->count_in_bars_remaining;
    } else if (screen_id == 1) {
        render_mixer_view(hs, preset, preset_idx, screen_switched || force_redraw);
    } else if (screen_id == 2) {
        render_style_view(hs, preset, screen_switched || force_redraw);
    } else {
        render_feel_view(hs, screen_switched || force_redraw);
    }

    g_last_bar = hs->current_bar;
    g_last_screen = screen_id;
    g_last_preset = preset_idx;
    g_last_key = hs->root_semitone;
    g_last_algo = preset->ep_tone_algo;
    g_last_bass_inst = preset->bass_instrument;
    g_last_drum = preset->drum_kit;
    g_last_lead = preset->lead_instrument;
    g_last_lead_play = preset->lead_play_style;
    g_last_easy_lead = hs->easy_lead;
    g_last_playing = hs->is_playing;
    g_last_groove = preset->groove_style;
    g_last_variation = hs->variation;
    g_last_heat = hs->heat_level;
    g_last_bpm = preset->bpm;
    g_last_form = hs->form_mode;
}

void ui_render_splash(void) {
    hal_blit_sprite_2x(0, 0, SPLASH_ART_W, SPLASH_ART_H, SPLASH_ART_PAL, SPLASH_ART_DATA);
    hal_fill_rect(0, 220, 240, 20, RGB(12, 12, 18));
    hal_draw_text_box(0, 222, 240, 0, "BRIAN PROVOST  *  V2.2", RGB(240, 195, 90), 1);
}
