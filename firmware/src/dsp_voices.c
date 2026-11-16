#include "dsp_voices.h"
#include "hal_adapter.h"

static const uint32_t G_MIDI_PHASE_INC[128] = {
    796254u, 843601u, 893765u, 946911u, 1003217u, 1062871u,
    1126073u, 1193033u, 1263974u, 1339134u, 1418763u, 1503127u,
    1592507u, 1687203u, 1787529u, 1893821u, 2006434u, 2125742u,
    2252146u, 2386065u, 2527948u, 2678268u, 2837526u, 3006254u,
    3185015u, 3374406u, 3575058u, 3787642u, 4012867u, 4251485u,
    4504291u, 4772130u, 5055896u, 5356535u, 5675051u, 6012507u,
    6370030u, 6748811u, 7150117u, 7575285u, 8025735u, 8502970u,
    9008582u, 9544261u, 10111792u, 10713070u, 11350103u, 12025015u,
    12740059u, 13497623u, 14300233u, 15150569u, 16051469u, 17005939u,
    18017165u, 19088521u, 20223584u, 21426141u, 22700205u, 24050030u,
    25480119u, 26995246u, 28600467u, 30301139u, 32102938u, 34011878u,
    36034330u, 38177043u, 40447168u, 42852281u, 45400411u, 48100060u,
    50960238u, 53990491u, 57200933u, 60602278u, 64205876u, 68023757u,
    72068660u, 76354085u, 80894335u, 85704563u, 90800821u, 96200119u,
    101920476u, 107980983u, 114401866u, 121204555u, 128411753u, 136047513u,
    144137319u, 152708170u, 161788671u, 171409126u, 181601643u, 192400238u,
    203840952u, 215961966u, 228803732u, 242409110u, 256823506u, 272095026u,
    288274639u, 305416341u, 323577341u, 342818251u, 363203285u, 384800477u,
    407681904u, 431923931u, 457607465u, 484818220u, 513647012u, 544190053u,
    576549277u, 610832681u, 647154683u, 685636503u, 726406571u, 769600953u,
    815363807u, 863847862u, 915214929u, 969636441u, 1027294024u, 1088380105u,
    1153098554u, 1221665363u
};

typedef struct {
    uint32_t phase;
    uint32_t phase_inc;
    int32_t  env_q15;
    int32_t  target_env;
    uint32_t gate_samples;
    uint16_t pluck_samples;
    uint8_t  active;
} BassVoice;

typedef struct {
    uint32_t phase_c;
    uint32_t phase_m;
    uint32_t inc_c;
    uint32_t inc_m;
    int32_t  env_q15;
    uint32_t gate_samples;
    uint8_t  active;
} KeysVoice;

#define MAX_LEAD_VOICES 4

typedef struct {
    uint32_t phase;
    uint32_t phase2;
    uint32_t phase_inc;
    uint32_t target_inc;
    int32_t  env_q15;
    int32_t  target_env;
    uint32_t gate_samples;
    uint8_t  active;
    uint8_t  midi_note;
    int32_t  filter_lp;
    uint32_t vib_phase;
    uint16_t vib_delay;
    uint16_t bend_samples;
} LeadVoice;

static BassVoice g_bass;
static KeysVoice g_keys[4];
static LeadVoice g_lead[MAX_LEAD_VOICES];
static uint8_t   g_lead_play_style = LEAD_PLAY_POLY;
static uint8_t   g_lead_tuning = 0;
static uint8_t   g_mono_stack[8];
static uint8_t   g_mono_len = 0;
static int32_t   g_bass_lp = 0;
static int32_t   g_drum_env[4];
static uint16_t  g_drum_decay[4] = { 32763, 32756, 32720, 32732 };
static uint32_t  g_drum_pitch[4];

// 4-Comb Schroeder Room Reverb Buffers (Prime lengths)
static int16_t   g_comb_buf0[743];
static int16_t   g_comb_buf1[887];
static int16_t   g_comb_buf2[1013];
static int16_t   g_comb_buf3[1153];
static uint16_t  g_comb_ptr0 = 0, g_comb_ptr1 = 0, g_comb_ptr2 = 0, g_comb_ptr3 = 0;
static int16_t   g_comb_lp0 = 0, g_comb_lp1 = 0, g_comb_lp2 = 0, g_comb_lp3 = 0;
static uint32_t  g_lfsr = 0xACE1u;
static int32_t   g_eq_lp = 0;

void dsp_init(void) {
    g_bass.active = 0;
    g_bass.phase = 0;
    g_bass.env_q15 = 0;
    g_bass.gate_samples = 0;
    g_bass_lp = 0;
    g_eq_lp = 0;
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        g_lead[v].active = 0;
        g_lead[v].phase = 0;
        g_lead[v].phase2 = 0;
        g_lead[v].phase_inc = 0;
        g_lead[v].target_inc = 0;
        g_lead[v].env_q15 = 0;
        g_lead[v].gate_samples = 0;
        g_lead[v].filter_lp = 0;
        g_lead[v].vib_phase = 0;
        g_lead[v].vib_delay = 0;
        g_lead[v].bend_samples = 0;
        g_lead[v].midi_note = 0xFF;
    }
    g_mono_len = 0;
    for (int k = 0; k < 4; k++) {
        g_keys[k].active = 0;
        g_keys[k].phase_c = 0;
        g_keys[k].phase_m = 0;
        g_keys[k].env_q15 = 0;
        g_keys[k].gate_samples = 0;
    }
    for (int d = 0; d < 4; d++) {
        g_drum_env[d] = 0;
        g_drum_pitch[d] = 0;
    }
    for (int i = 0; i < 743; i++) g_comb_buf0[i] = 0;
    for (int i = 0; i < 887; i++) g_comb_buf1[i] = 0;
    for (int i = 0; i < 1013; i++) g_comb_buf2[i] = 0;
    for (int i = 0; i < 1153; i++) g_comb_buf3[i] = 0;
    g_comb_ptr0 = 0; g_comb_ptr1 = 0; g_comb_ptr2 = 0; g_comb_ptr3 = 0;
    g_comb_lp0 = 0; g_comb_lp1 = 0; g_comb_lp2 = 0; g_comb_lp3 = 0;
    g_lfsr = 0xACE1u;
}

void dsp_all_notes_off(void) {
    g_bass.active = 0;
    for (int k = 0; k < 4; k++) g_keys[k].active = 0;
    for (int d = 0; d < 4; d++) g_drum_env[d] = 0;
    g_mono_len = 0;
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        g_lead[v].active = 0;
        g_lead[v].env_q15 = 0;
        g_lead[v].gate_samples = 0;
        g_lead[v].bend_samples = 0;
    }
}

static inline uint32_t get_scaled_inc(uint8_t midi_note, uint8_t tuning_mode) {
    uint32_t inc = G_MIDI_PHASE_INC[midi_note & 0x7F];
    if (tuning_mode == 0) return inc;
    uint32_t mult = (tuning_mode == 1) ? 32173u : 32917u;
    return ((inc >> 15) * mult) + (((inc & 0x7FFF) * mult) >> 15);
}

void dsp_note_on_bass(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode) {
    g_bass.phase_inc = get_scaled_inc(midi_note, tuning_mode);
    g_bass.target_env = (int32_t)velocity * 256;
    if (!g_bass.active) {
        g_bass.phase = 0;
        g_bass.env_q15 = 0;
    } else {
        g_bass.env_q15 >>= 1; // Legato tie
    }
    g_bass.gate_samples = 17640; // ~400 ms walking bass gate
    g_bass.pluck_samples = 2200; // ~50 ms initial pluck snap
    g_bass.active = 1;
}

void dsp_note_on_keys(const Voicing *voicing, uint8_t velocity, uint8_t tuning_mode) {
    for (uint8_t i = 0; i < voicing->count && i < 4; i++) {
        g_keys[i].inc_c = get_scaled_inc(voicing->notes[i], tuning_mode);
        g_keys[i].inc_m = g_keys[i].inc_c * 2;
        g_keys[i].env_q15 = (int32_t)velocity * 256;
        g_keys[i].gate_samples = 15000; // ~340 ms gate hold
        if (!g_keys[i].active) {
            g_keys[i].phase_c = 0;
            g_keys[i].phase_m = 0;
        }
        g_keys[i].active = 1;
    }
    for (uint8_t i = voicing->count; i < 4; i++) {
        g_keys[i].active = 0;
    }
}

void dsp_set_lead_play_style(uint8_t play_style) {
    g_lead_play_style = play_style % NUM_LEAD_PLAY_STYLES;
    if (g_lead_play_style != LEAD_PLAY_LEGATO_GLIDE) {
        g_mono_len = 0;
    }
}

static void lead_trigger_voice(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode, uint8_t scoop_bend) {
    int target_v = -1;
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        if (g_lead[v].active && g_lead[v].midi_note == midi_note) {
            target_v = v;
            break;
        }
    }
    if (target_v < 0) {
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (!g_lead[v].active) {
                target_v = v;
                break;
            }
        }
    }
    if (target_v < 0) {
        int32_t min_env = 0x7FFFFFFF;
        target_v = 0;
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (g_lead[v].env_q15 < min_env) {
                min_env = g_lead[v].env_q15;
                target_v = v;
            }
        }
    }

    LeadVoice *lv = &g_lead[target_v];
    uint32_t inc = get_scaled_inc(midi_note, tuning_mode);
    lv->target_inc = inc;
    lv->midi_note = midi_note;
    lv->target_env = (int32_t)velocity * 256;
    if (!lv->active) {
        lv->phase = 0;
        lv->phase2 = 0;
        lv->env_q15 = 0;
        lv->vib_delay = 1800; // ~40 ms delay before expressive blues vibrato onset
        lv->vib_phase = 0;
        lv->filter_lp = 0;
        lv->phase_inc = scoop_bend ? (inc - (inc >> 4) - (inc >> 5)) : inc;
        lv->bend_samples = scoop_bend ? 2200u : 0u;
    } else {
        lv->env_q15 = (lv->env_q15 * 3 + lv->target_env) / 4;
        lv->vib_delay = 1200;
        lv->phase_inc = inc;
        lv->bend_samples = 0;
    }
    lv->gate_samples = 44100 * 10;
    lv->active = 1;
}

void dsp_note_on_lead(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode) {
    g_lead_tuning = tuning_mode;

    if (g_lead_play_style == LEAD_PLAY_LEGATO_GLIDE) {
        uint8_t was_held = (g_mono_len > 0 && g_lead[0].active);
        uint8_t found = 0;
        for (uint8_t i = 0; i < g_mono_len; i++) {
            if (g_mono_stack[i] == midi_note) { found = 1; break; }
        }
        if (!found && g_mono_len < 8) {
            g_mono_stack[g_mono_len++] = midi_note;
        }
        for (int v = 1; v < MAX_LEAD_VOICES; v++) {
            g_lead[v].gate_samples = 0;
        }
        LeadVoice *lv = &g_lead[0];
        uint32_t inc = get_scaled_inc(midi_note, tuning_mode);
        lv->target_inc = inc;
        lv->midi_note = midi_note;
        lv->target_env = (int32_t)velocity * 256;
        lv->gate_samples = 44100 * 10;
        if (!was_held) {
            lv->phase = 0;
            lv->phase2 = 0;
            lv->env_q15 = 0;
            lv->phase_inc = inc;
            lv->vib_delay = 1800;
            lv->vib_phase = 0;
            lv->filter_lp = 0;
            lv->bend_samples = 0;
        } else {
            // Legato glide: preserve envelope and slew phase_inc toward target_inc
            lv->vib_delay = 1200;
            lv->bend_samples = 2800u;
        }
        lv->active = 1;
        return;
    }

    if (g_lead_play_style == LEAD_PLAY_OCTAVE_STACK) {
        lead_trigger_voice(midi_note, velocity, tuning_mode, 0);
        uint8_t oct_note = (midi_note <= 115) ? (uint8_t)(midi_note + 12) : midi_note;
        if (oct_note != midi_note) {
            lead_trigger_voice(oct_note, (uint8_t)((velocity * 3) >> 2), tuning_mode, 0);
        }
        return;
    }

    lead_trigger_voice(midi_note, velocity, tuning_mode, (g_lead_play_style == LEAD_PLAY_BLUES_BEND));
}

void dsp_note_off_lead(uint8_t midi_note) {
    if (midi_note == 0xFF) {
        g_mono_len = 0;
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (g_lead[v].active) g_lead[v].gate_samples = 0;
        }
        return;
    }

    if (g_lead_play_style == LEAD_PLAY_LEGATO_GLIDE) {
        for (uint8_t i = 0; i < g_mono_len; i++) {
            if (g_mono_stack[i] == midi_note) {
                for (uint8_t j = i; j + 1 < g_mono_len; j++) {
                    g_mono_stack[j] = g_mono_stack[j + 1];
                }
                g_mono_len--;
                break;
            }
        }
        if (g_mono_len > 0 && g_lead[0].active) {
            uint8_t prev = g_mono_stack[g_mono_len - 1];
            g_lead[0].midi_note = prev;
            g_lead[0].target_inc = get_scaled_inc(prev, g_lead_tuning);
            g_lead[0].bend_samples = 2800u;
        } else {
            g_lead[0].gate_samples = 0;
        }
        return;
    }

    uint8_t oct_note = (g_lead_play_style == LEAD_PLAY_OCTAVE_STACK && midi_note <= 115)
                       ? (uint8_t)(midi_note + 12) : 0xFF;
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        if (g_lead[v].active) {
            if (g_lead[v].midi_note == midi_note || (oct_note != 0xFF && g_lead[v].midi_note == oct_note)) {
                g_lead[v].gate_samples = 0; // Natural release phase
            }
        }
    }
}

void dsp_trigger_drum(uint8_t mask, const uint8_t *drum_vel, uint8_t open_hat) {
    for (int d = 0; d < 4; d++) {
        if (mask & (1 << d)) {
            g_drum_env[d] = (int32_t)drum_vel[d] * 256;
            g_drum_pitch[d] = 0;
            if (d == 2) {
                g_drum_decay[2] = open_hat ? 32760 : 32720;
            }
        }
    }
}

static inline int32_t q15_mul_trunc(int32_t a, int32_t b) {
    int32_t p = a * b;
    return (p >= 0) ? (p >> 15) : -((-p) >> 15);
}

static inline int32_t div2_trunc(int32_t x) {
    return (x >= 0) ? (x >> 1) : -((-x) >> 1);
}

static inline int32_t div4_trunc(int32_t x) {
    return (x >= 0) ? (x >> 2) : -((-x) >> 2);
}

void dsp_render_block(int32_t *buf, uint32_t num_frames, const PresetConfig *preset) {
    for (uint32_t i = 0; i < num_frames; i++) {
        int32_t mix = 0;

        // 1. Bass Synth Engine (4 Selectable Instruments)
        if (g_bass.active) {
            g_bass.phase += g_bass.phase_inc;
            int16_t b_fund = hal_sine_q15(g_bass.phase);
            int16_t b_harm = hal_sine_q15(g_bass.phase * 2);
            int16_t b_third = hal_sine_q15(g_bass.phase * 3);
            int32_t b_sample;

            if (g_bass.pluck_samples > 0) g_bass.pluck_samples--;

            switch (preset->bass_instrument % NUM_BASS_INSTRUMENTS) {
                case BASS_UPRIGHT:
                    b_sample = (b_fund * 3 / 4) + (b_harm >> 2);
                    if (g_bass.pluck_samples > 0) {
                        b_sample += (int32_t)g_bass.pluck_samples * 6;
                    }
                    break;
                case BASS_MUTED_TICTAC:
                    b_sample = (b_fund >> 2) + (b_harm >> 1) + (b_third >> 2);
                    break;
                case BASS_MOOG_FUNK: {
                    int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                    int32_t saw = (b_fund >> 1) + (b_harm >> 2) + (b_third >> 3) + (b_fourth >> 4);
                    int32_t cutoff = 3000 + ((g_bass.env_q15 * 24000) >> 15);
                    g_bass_lp += ((saw - g_bass_lp) * cutoff) >> 15;
                    b_sample = g_bass_lp;
                    break;
                }
                case BASS_JAZZ_60S: { // 60s Jazz Bass: bridge pickup mid growl
                    int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                    b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 2) + (b_fourth >> 3);
                    break;
                }
                case BASS_FLATWOUND: // Flatwound Motown: warm round fundamental
                    b_sample = (b_fund * 3 / 4) + (b_harm >> 3) + (b_third >> 4);
                    break;
                case BASS_AMPEG_GROWL: { // Ampeg Tube Growl: warm asymmetric saturation
                    int32_t raw = (b_fund * 3 / 4) + (b_harm >> 1) + (b_third >> 2);
                    if (raw > 14000) raw = 14000 + ((raw - 14000) >> 2);
                    else if (raw < -14000) raw = -14000 + ((raw + 14000) >> 2);
                    b_sample = raw;
                    break;
                }
                case BASS_SUB_OCTAVE: // Sub Octave: deep fundamental + clean octave blend
                    b_sample = (b_fund * 7 / 8) + (b_harm >> 2);
                    break;
                case BASS_LATELY_FM: { // DX7/TX81Z Lately FM Bass: snappy 2:1 FM punch
                    uint32_t fm_mod = (uint32_t)((((int32_t)b_harm * g_bass.env_q15) >> 15) * 18000);
                    b_sample = hal_sine_q15(g_bass.phase + fm_mod);
                    break;
                }
                case BASS_SLAP_POP: { // Karplus/Slap Pop: deep thump + bright snap transient
                    int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                    int32_t slap = (g_bass.pluck_samples > 0) ? ((b_third >> 1) + (b_fourth >> 1)) : (b_harm >> 3);
                    b_sample = (b_fund * 3 / 4) + slap;
                    break;
                }
                case BASS_HOFNER: // Hollow woody short-decay Beatle bass
                    b_sample = (b_fund * 7 / 8) + (b_harm >> 3);
                    break;
                case BASS_ACID_303: { // Resonant squelchy synth bass
                    int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                    int32_t saw = (b_fund >> 1) + (b_harm >> 2) + (b_third >> 2) + (b_fourth >> 3);
                    int32_t cutoff = 2000 + ((g_bass.env_q15 * 28000) >> 15);
                    g_bass_lp += ((saw - g_bass_lp) * cutoff) >> 15;
                    b_sample = g_bass_lp + ((saw - g_bass_lp) >> 2);
                    break;
                }
                case BASS_TUBA: // New Orleans brass tuba
                    b_sample = (b_fund * 5 / 8) + (b_harm >> 2) + (b_third >> 3);
                    break;
                case BASS_PICK_ROCK: { // Aggressive picked rock-blues bass
                    int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                    int32_t pick = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 2) + (b_fourth >> 2);
                    if (pick > 18000) pick = 18000;
                    else if (pick < -18000) pick = -18000;
                    b_sample = pick;
                    break;
                }
                case BASS_P_BASS:
                default:
                    if (g_bass.pluck_samples > 0) {
                        b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 2);
                    } else {
                        b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 3);
                    }
                    break;
            }

            int32_t b_amp = (g_bass.env_q15 * preset->bass_vol) >> 15;
            mix += (b_sample * b_amp) >> 7; // Driving, punchy walking bass

            // Attack micro-ramp, then clear target_env so plucked-string decay runs freely
            if (g_bass.target_env > 0) {
                g_bass.env_q15 += (g_bass.target_env - g_bass.env_q15 + 15) >> 4;
                if (g_bass.env_q15 >= g_bass.target_env - 64) {
                    g_bass.target_env = 0;
                }
            } else if (g_bass.gate_samples > 0) {
                g_bass.gate_samples--;
                static const int32_t BASS_DECAYS[NUM_BASS_INSTRUMENTS] = {
                    32766, // P-BASS (~258 ms)
                    32764, // UPRIGHT (~129 ms)
                    32761, // MUTED BOOGIE (~73 ms)
                    32765, // MOOG FUNK (~172 ms)
                    32766, // 60S JAZZ (~258 ms)
                    32765, // FLATWOUND (~172 ms)
                    32766, // AMPEG TUBE (~258 ms)
                    32766, // SUB OCTAVE (~258 ms)
                    32764, // LATELY FM (~129 ms)
                    32763, // SLAP POP (~103 ms)
                    32762, // HOFNER (~86 ms)
                    32764, // ACID 303 (~129 ms)
                    32765, // TUBA (~172 ms)
                    32766  // PICK ROCK (~258 ms)
                };
                g_bass.env_q15 = (g_bass.env_q15 * BASS_DECAYS[preset->bass_instrument % NUM_BASS_INSTRUMENTS]) >> 15;
            } else {
                g_bass.env_q15 = (g_bass.env_q15 * 32745) >> 15; // Smooth ~22 ms half-life release tail
            }
            if (g_bass.target_env == 0 && g_bass.env_q15 < 100) g_bass.active = 0;
        }

        // 2. Keys Synth Engine: 4-Voice Polyphony across 8 Instruments
        for (int k = 0; k < 4; k++) {
            if (g_keys[k].active) {
                int16_t car = 0;
                int32_t m_env = (g_keys[k].env_q15 * 3) / 4;

                switch (preset->ep_tone_algo % NUM_KEYS_INSTRUMENTS) {
                    case KEYS_RHODES: { // 0: Rhodes FM
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c;
                        int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 16000);
                        car = hal_sine_q15(g_keys[k].phase_c + pmod);
                        break;
                    }
                    case KEYS_BRIGHT_EP: { // 1: Bright FM EP
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c * 2;
                        int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 24000);
                        car = hal_sine_q15(g_keys[k].phase_c + pmod);
                        break;
                    }
                    case KEYS_WURLI: { // 2: Wurli 200A
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c;
                        int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 20000);
                        int32_t w = hal_sine_q15(g_keys[k].phase_c + pmod);
                        if (w > 16000) w = 16000 + ((w - 16000) >> 1);
                        else if (w < -16000) w = -16000 + ((w + 16000) >> 1);
                        car = (int16_t)w;
                        break;
                    }
                    case KEYS_B3_CLEAN: { // 3: Clean B3 Organ (88 8000 000)
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t h1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t h2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        int16_t h3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        car = (h1 >> 1) + (h2 >> 2) + (h3 >> 3);
                        break;
                    }
                    case KEYS_B3_DIRTY: { // 4: Dirty Gospel B3 Organ (88 8800 000)
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t h1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t h2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        int16_t h3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        int16_t h4 = hal_sine_q15(g_keys[k].phase_c * 4);
                        int32_t organ = h1 + h2 + (h3 >> 1) + (h4 >> 2);
                        if (organ > 22000) organ = 22000;
                        else if (organ < -22000) organ = -22000;
                        car = (int16_t)(organ >> 1);
                        break;
                    }
                    case KEYS_CLAVINET: { // 5: Funk Clavinet D6
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c * 3;
                        int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)mod * g_keys[k].env_q15) >> 15) * 28000);
                        car = hal_sine_q15(g_keys[k].phase_c + pmod);
                        break;
                    }
                    case KEYS_ARCHTOP_GUITAR: { // 6: Archtop Rhythm Guitar
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t g1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t g2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        int16_t g3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        car = (g1 * 5 / 8) + (g2 >> 2) + (g3 >> 3);
                        break;
                    }
                    case KEYS_BLUES_HORNS: { // 7: Blues Horn Section
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t h1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t h2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        int16_t h3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        car = (h1 >> 1) + (h2 * 3 / 8) + (h3 >> 3);
                        break;
                    }
                    case KEYS_HONKY_TONK: { // 8: Honky-Tonk Upright Piano
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c + (g_keys[k].inc_c >> 7);
                        int16_t p1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t p2 = hal_sine_q15(g_keys[k].phase_m);
                        int16_t p3 = hal_sine_q15(g_keys[k].phase_c * 2);
                        car = (p1 >> 1) + (p2 * 3 / 8) + (p3 >> 3);
                        break;
                    }
                    case KEYS_TREMOLO_EP: { // 9: Vintage Tremolo Suitcase EP
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c;
                        int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 16000);
                        int16_t raw_ep = hal_sine_q15(g_keys[k].phase_c + pmod);
                        int16_t trem = hal_sine_q15((uint32_t)i * 440000u);
                        int32_t trem_gain = 24576 + (trem >> 3);
                        car = (int16_t)((raw_ep * trem_gain) >> 15);
                        break;
                    }
                    case KEYS_RESONATOR: { // 10: Delta Resonator / Dobro Guitar
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c * 4;
                        int16_t m4 = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t pmod = (uint32_t)((((int32_t)m4 * g_keys[k].env_q15) >> 15) * 22000);
                        int16_t res = hal_sine_q15(g_keys[k].phase_c + pmod);
                        int16_t c2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        car = (res * 3 / 4) + (c2 >> 2);
                        break;
                    }
                    case KEYS_LESLIE_ORGAN: { // 11: Rotary Leslie Swell Organ
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += 580000u;
                        int16_t rot = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t rot_phase = g_keys[k].phase_c + (uint32_t)(rot * 12);
                        int16_t o1 = hal_sine_q15(rot_phase);
                        int16_t o2 = hal_sine_q15(rot_phase * 2);
                        int16_t o3 = hal_sine_q15(rot_phase * 3);
                        car = (o1 >> 1) + (o2 * 3 / 8) + (o3 >> 3);
                        break;
                    }
                    case KEYS_ACCORDION: { // 12: Cajun / Zydeco Detuned Accordion
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c + (g_keys[k].inc_c >> 7);
                        int16_t a1 = hal_sine_q15(g_keys[k].phase_c) + (hal_sine_q15(g_keys[k].phase_c * 3) >> 2);
                        int16_t a2 = hal_sine_q15(g_keys[k].phase_m) + (hal_sine_q15(g_keys[k].phase_m * 3) >> 2);
                        car = (a1 >> 1) + (a2 >> 1);
                        break;
                    }
                    case KEYS_VIBES: { // 13: Glassy 1:4 FM Vibraphone with tremolo
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c * 4;
                        int16_t m_v = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t v_mod = (uint32_t)((((int32_t)m_v * g_keys[k].env_q15) >> 15) * 6000);
                        car = hal_sine_q15(g_keys[k].phase_c + v_mod);
                        break;
                    }
                    case KEYS_FARFISA: { // 14: Bright 60s Combo Organ
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t f1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t f2 = hal_sine_q15(g_keys[k].phase_c * 2);
                        int16_t f3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        int16_t f4 = hal_sine_q15(g_keys[k].phase_c * 4);
                        car = (f1 >> 2) + (f2 >> 2) + (f3 >> 2) + (f4 >> 3);
                        break;
                    }
                    case KEYS_MELLOTRON: { // 15: Warm Tape Strings Pad
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c + (g_keys[k].inc_c >> 8);
                        int16_t s1 = hal_sine_q15(g_keys[k].phase_c) + (hal_sine_q15(g_keys[k].phase_c * 2) >> 2);
                        int16_t s2 = hal_sine_q15(g_keys[k].phase_m) + (hal_sine_q15(g_keys[k].phase_m * 2) >> 2);
                        car = (s1 >> 1) + (s2 >> 1);
                        break;
                    }
                    case KEYS_CP70: { // 16: CP-70 Electric Grand
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        g_keys[k].phase_m += g_keys[k].inc_c * 2;
                        int16_t m_cp = hal_sine_q15(g_keys[k].phase_m);
                        uint32_t cp_mod = (uint32_t)((((int32_t)m_cp * g_keys[k].env_q15) >> 15) * 14000);
                        car = (hal_sine_q15(g_keys[k].phase_c + cp_mod) * 3 / 4) + (hal_sine_q15(g_keys[k].phase_c * 3) >> 3);
                        break;
                    }
                    case KEYS_HARMONIUM: // 17: Woody Pump Organ / Harmonium
                    default: {
                        g_keys[k].phase_c += g_keys[k].inc_c;
                        int16_t h1 = hal_sine_q15(g_keys[k].phase_c);
                        int16_t h3 = hal_sine_q15(g_keys[k].phase_c * 3);
                        int16_t h5 = hal_sine_q15(g_keys[k].phase_c * 5);
                        car = (h1 * 5 / 8) + (h3 >> 2) + (h5 >> 4);
                        break;
                    }
                }

                int32_t k_amp = (g_keys[k].env_q15 * preset->keys_vol) >> 15;
                mix += (car * k_amp) >> 8;

                if (g_keys[k].gate_samples > 0) {
                    g_keys[k].gate_samples--;
                    uint8_t k_algo = preset->ep_tone_algo % NUM_KEYS_INSTRUMENTS;
                    if (k_algo != KEYS_B3_CLEAN && k_algo != KEYS_B3_DIRTY && k_algo != KEYS_LESLIE_ORGAN &&
                        k_algo != KEYS_ACCORDION && k_algo != KEYS_FARFISA && k_algo != KEYS_MELLOTRON && k_algo != KEYS_HARMONIUM) {
                        g_keys[k].env_q15 = (g_keys[k].env_q15 * 32766) >> 15;
                    }
                } else {
                    g_keys[k].env_q15 = (g_keys[k].env_q15 * 32650) >> 15;
                }
                if (g_keys[k].env_q15 < 100) g_keys[k].active = 0;
            }
        }

        // 3. Synthesized Drums (12 Selectable Kits)
        static const uint32_t KICK_BASE_INC[NUM_DRUM_KITS] = {
            7500000u, 6800000u, 5800000u, 8200000u,
            7200000u, 7800000u, 7000000u, 7000000u,
            6200000u, 6600000u, 8000000u, 5400000u
        };
        static const uint32_t KICK_SWEEP_MUL[NUM_DRUM_KITS] = {
            420u, 320u, 480u, 440u,
            390u, 360u, 340u, 520u,
            350u, 400u, 500u, 300u
        };
        uint8_t dkit = preset->drum_kit % NUM_DRUM_KITS;

        // Kick (0)
        if (g_drum_env[0] > 0) {
            uint32_t kick_inc = KICK_BASE_INC[dkit] + (uint32_t)g_drum_env[0] * KICK_SWEEP_MUL[dkit];
            g_drum_pitch[0] += kick_inc;
            int16_t k_sample = hal_sine_q15(g_drum_pitch[0]);
            if (dkit == DRUM_HEAVY_ROCK) {
                k_sample = (int16_t)((k_sample * 5 / 4) > 32767 ? 32767 : (k_sample * 5 / 4) < -32767 ? -32767 : (k_sample * 5 / 4));
            }
            int32_t d0_amp = (g_drum_env[0] * preset->drums_vol) >> 15;
            mix += (k_sample * d0_amp) >> 7;
            g_drum_env[0] = (g_drum_env[0] * g_drum_decay[0]) >> 15;
        }

        // Snare (1)
        if (g_drum_env[1] > 0) {
            g_lfsr = (g_lfsr >> 1) ^ (-(g_lfsr & 1u) & 0xB400u);
            int16_t noise_sample = (int16_t)(g_lfsr & 0x7FFF);
            int16_t s_sample;
            static const uint32_t SNARE_BODY_INC[NUM_DRUM_KITS] = {
                17540000u, 15000000u, 22000000u, 19500000u,
                16200000u, 18400000u, 21000000u, 15800000u,
                20000000u, 19000000u, 17000000u, 14500000u
            };
            g_drum_pitch[1] += SNARE_BODY_INC[dkit];
            int16_t body = hal_sine_q15(g_drum_pitch[1]);

            if (dkit == DRUM_BRUSH || dkit == DRUM_LOFI_BOX) {
                s_sample = (dkit == DRUM_LOFI_BOX) ? ((noise_sample >> 2) + (body >> 1)) : (noise_sample >> 1);
            } else if (dkit == DRUM_RETRO_808 || dkit == DRUM_LINN_GATE) {
                s_sample = (noise_sample * 3 / 4) + (body >> 2); // Snappy 808 / gated snare
            } else if (dkit == DRUM_STAX_TIGHT || dkit == DRUM_DRY_VINTAGE || dkit == DRUM_NEW_ORLEANS) {
                s_sample = (noise_sample * 5 / 8) + (body >> 1); // Tight dry / marching crack
            } else if (dkit == DRUM_CHICAGO_ROOM || dkit == DRUM_HEAVY_ROCK) {
                s_sample = (noise_sample * 3 / 8) + (body * 5 / 8); // Fat woody body
            } else if (dkit == DRUM_JAZZ_CLUB || dkit == DRUM_CR78) {
                s_sample = (noise_sample * 5 / 8) + (body >> 2); // Crisp snappy snare
            } else {
                s_sample = (noise_sample >> 1) + (body >> 1); // Studio snare
            }
            int32_t d1_amp = (g_drum_env[1] * preset->drums_vol) >> 15;
            mix += (s_sample * d1_amp) >> 7;
            g_drum_env[1] = (g_drum_env[1] * g_drum_decay[1]) >> 15;
        }

        // Hi-Hat / Ride (2)
        if (g_drum_env[2] > 0) {
            g_lfsr = (g_lfsr >> 1) ^ (-(g_lfsr & 1u) & 0xB400u);
            int16_t n_sample = (int16_t)(g_lfsr & 0xFFFF);
            int32_t d2_amp = (g_drum_env[2] * preset->drums_vol) >> 15;
            mix += (n_sample * d2_amp) >> 9; // Sit hats behind kick & snare
            g_drum_env[2] = (g_drum_env[2] * g_drum_decay[2]) >> 15;
        }

        // Rimshot (3)
        if (g_drum_env[3] > 0) {
            static const uint32_t RIM_INC[NUM_DRUM_KITS] = {
                12000000u, 8500000u, 16000000u, 13000000u,
                11000000u, 10500000u, 14500000u, 12500000u,
                14000000u, 15500000u, 13500000u, 9000000u
            };
            g_drum_pitch[3] += RIM_INC[dkit];
            int16_t r_sample = hal_sine_q15(g_drum_pitch[3]);
            int32_t d3_amp = (g_drum_env[3] * preset->drums_vol) >> 15;
            mix += (r_sample * d3_amp) >> 7;
            g_drum_env[3] = (g_drum_env[3] * g_drum_decay[3]) >> 15;
        }

        // 4. Solo Lead Voice (4-Voice Polyphony across 14 Lead Instruments + 4 Play Styles)
        g_lead_play_style = preset->lead_play_style % NUM_LEAD_PLAY_STYLES;
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (g_lead[v].active) {
                if (g_lead[v].bend_samples > 0) {
                    g_lead[v].bend_samples--;
                    int32_t diff = (int32_t)g_lead[v].target_inc - (int32_t)g_lead[v].phase_inc;
                    g_lead[v].phase_inc = (uint32_t)((int32_t)g_lead[v].phase_inc + (diff >> 9));
                    if (g_lead[v].bend_samples == 0) {
                        g_lead[v].phase_inc = g_lead[v].target_inc;
                    }
                }
                uint32_t cur_inc = g_lead[v].phase_inc;
                if (g_lead[v].vib_delay > 0) {
                    g_lead[v].vib_delay--;
                } else {
                    g_lead[v].vib_phase += 520000u; // ~5.3 Hz expressive blues vibrato
                    int16_t vib = hal_sine_q15(g_lead[v].vib_phase);
                    int32_t delta = (int32_t)((g_lead[v].phase_inc >> 10) * vib) >> 11;
                    cur_inc = (uint32_t)((int32_t)g_lead[v].phase_inc + delta);
                }
                uint32_t prev_phase = g_lead[v].phase;
                g_lead[v].phase += cur_inc;

                int32_t lead_sample = 0;
                switch (preset->lead_instrument % NUM_LEAD_INSTRUMENTS) {
                    case LEAD_BLUES_GUITAR: { // 0: Overdriven Blues Guitar
                        int16_t w1 = hal_sine_q15(g_lead[v].phase);
                        int16_t w2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t w3 = hal_sine_q15(g_lead[v].phase * 3);
                        int32_t g_raw = (w1 >> 1) + (w2 >> 2) + (w3 >> 3);
                        int32_t g_driven = (g_raw * 5) >> 2;
                        if (g_driven > 24000) g_driven = 24000;
                        else if (g_driven < -24000) g_driven = -24000;
                        lead_sample = g_driven;
                        break;
                    }
                    case LEAD_HARMONICA: { // 1: Chicago Blues Harp
                        int16_t h1 = hal_sine_q15(g_lead[v].phase);
                        int16_t h3 = hal_sine_q15(g_lead[v].phase * 3);
                        int16_t h5 = hal_sine_q15(g_lead[v].phase * 5);
                        int32_t reed = (h1 >> 1) + (h3 >> 2) + (h5 >> 3);
                        if (reed > 20000) reed = 20000;
                        else if (reed < -20000) reed = -20000;
                        lead_sample = reed;
                        break;
                    }
                    case LEAD_TENOR_SAX: { // 2: Tenor Sax
                        int16_t s1 = hal_sine_q15(g_lead[v].phase);
                        int16_t s2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t s3 = hal_sine_q15(g_lead[v].phase * 3);
                        int16_t breath = (int16_t)(g_lfsr & 0x07FF);
                        lead_sample = (s1 * 5 / 8) + (s2 >> 2) + (s3 >> 3) + breath;
                        break;
                    }
                    case LEAD_MOOG_LEAD: { // 3: Moog Solo Lead
                        g_lead[v].phase2 += cur_inc + (cur_inc >> 8);
                        int16_t m1 = hal_sine_q15(g_lead[v].phase);
                        int16_t m2 = hal_sine_q15(g_lead[v].phase2);
                        int32_t moog = (m1 >> 1) + (m2 >> 1);
                        g_lead[v].filter_lp += ((moog - g_lead[v].filter_lp) * 18000) >> 15;
                        lead_sample = g_lead[v].filter_lp;
                        break;
                    }
                    case LEAD_WARM_JAZZ: { // 4: Warm Jazz Hollowbody
                        int16_t j1 = hal_sine_q15(g_lead[v].phase);
                        int16_t j2 = hal_sine_q15(g_lead[v].phase * 2);
                        int32_t raw_jazz = (j1 * 3 / 4) + (j2 >> 3);
                        g_lead[v].filter_lp += ((raw_jazz - g_lead[v].filter_lp) * 12000) >> 15;
                        lead_sample = g_lead[v].filter_lp;
                        break;
                    }
                    case LEAD_FUZZ_OCTAVE: { // 5: Fuzz Octave Stompbox
                        int16_t f1 = hal_sine_q15(g_lead[v].phase);
                        int32_t rect = (f1 < 0) ? -f1 : f1;
                        int32_t fuzz = (f1 >> 1) + (rect >> 1);
                        fuzz = fuzz * 3;
                        if (fuzz > 22000) fuzz = 22000;
                        else if (fuzz < -22000) fuzz = -22000;
                        lead_sample = fuzz;
                        break;
                    }
                    case LEAD_MUTED_TRUMPET: { // 6: Muted Blues Trumpet
                        int16_t t1 = hal_sine_q15(g_lead[v].phase);
                        int16_t t3 = hal_sine_q15(g_lead[v].phase * 3);
                        int16_t t5 = hal_sine_q15(g_lead[v].phase * 5);
                        int32_t trpt = (t1 >> 2) + (t3 >> 1) + (t5 >> 2);
                        if (trpt > 20000) trpt = 20000;
                        else if (trpt < -20000) trpt = -20000;
                        lead_sample = trpt;
                        break;
                    }
                    case LEAD_PERC_ORGAN: { // 7: Percussive B3 Organ Lead
                        int16_t o1 = hal_sine_q15(g_lead[v].phase);
                        int16_t o2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t o3 = hal_sine_q15(g_lead[v].phase * 3);
                        lead_sample = (o1 >> 1) + (o2 >> 2) + (o3 * 3 / 8);
                        break;
                    }
                    case LEAD_TALKBOX: { // 8: VOSIM / Formant Talk-Box Lead
                        uint32_t fmt = 3u + (uint32_t)((g_lead[v].env_q15 >> 13) & 0x03);
                        int16_t f_osc = hal_sine_q15(g_lead[v].phase * fmt);
                        int16_t win = 16384 + (hal_sine_q15(g_lead[v].phase - 0x40000000u) >> 1);
                        lead_sample = (hal_sine_q15(g_lead[v].phase) >> 2) + ((f_osc * win) >> 15);
                        break;
                    }
                    case LEAD_SLIDE_GUITAR: { // 9: Glassy Slide / Lap Steel Guitar
                        int16_t s1 = hal_sine_q15(g_lead[v].phase);
                        int16_t s2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t s4 = hal_sine_q15(g_lead[v].phase * 4);
                        lead_sample = (s1 * 5 / 8) + (s2 >> 2) + (s4 >> 3);
                        break;
                    }
                    case LEAD_WAVEFOLDER: { // 10: Plaits-style Triangle Wavefolder Lead
                        int32_t ph_q15 = (int32_t)(g_lead[v].phase >> 16);
                        int32_t tri = (ph_q15 < 32768) ? (ph_q15 - 16384) : (49152 - ph_q15);
                        int32_t folded = tri * 3;
                        if (folded > 16384) folded = 32768 - folded;
                        else if (folded < -16384) folded = -32768 - folded;
                        if (folded > 16384) folded = 32768 - folded;
                        else if (folded < -16384) folded = -32768 - folded;
                        lead_sample = folded;
                        break;
                    }
                    case LEAD_HARD_SYNC: { // 11: OberVave-style Hard Sync Analog Lead
                        if (g_lead[v].phase < prev_phase) g_lead[v].phase2 = 0;
                        g_lead[v].phase2 += cur_inc + (cur_inc >> 1) + (uint32_t)((g_lead[v].env_q15 >> 15) * (cur_inc >> 2));
                        int16_t hs_raw = hal_sine_q15(g_lead[v].phase2);
                        g_lead[v].filter_lp += ((hs_raw - g_lead[v].filter_lp) * 16000) >> 15;
                        lead_sample = g_lead[v].filter_lp;
                        break;
                    }
                    case LEAD_SID_CHIP: { // 12: C64 SID PWM Chiptune Lead
                        uint32_t pw = 0x50000000u + ((uint32_t)hal_sine_q15(g_lead[v].vib_phase) << 12);
                        int32_t sq = (g_lead[v].phase < pw) ? 14000 : -14000;
                        lead_sample = sq & ~0x07FF; // Gritty 4-bit stepped SID crunch
                        break;
                    }
                    case LEAD_STEELPAN: // 13: Bright Percussive Steelpan / Mallet Lead
                    default: {
                        int16_t p1 = hal_sine_q15(g_lead[v].phase);
                        int16_t p2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t p4 = hal_sine_q15(g_lead[v].phase * 4);
                        lead_sample = (p1 * 5 / 8) + (p2 >> 2) + (p4 >> 3);
                        break;
                    }
                }

                uint8_t l_vol = preset->lead_vol > 0 ? preset->lead_vol : 115;
                int32_t l_amp = (g_lead[v].env_q15 * l_vol) >> 15;
                mix += (lead_sample * l_amp) >> 8;

                if (g_lead[v].gate_samples > 0) {
                    if (g_lead[v].env_q15 < g_lead[v].target_env) {
                        g_lead[v].env_q15 += (g_lead[v].target_env - g_lead[v].env_q15 + 15) >> 4;
                    }
                } else {
                    g_lead[v].env_q15 = (g_lead[v].env_q15 * 32400) >> 15;
                    if (g_lead[v].env_q15 < 100) g_lead[v].active = 0;
                }
            }
        }

        // 4-Comb Schroeder Room Reverb Parallel Summation
        int16_t c0 = g_comb_buf0[g_comb_ptr0];
        int16_t c1 = g_comb_buf1[g_comb_ptr1];
        int16_t c2 = g_comb_buf2[g_comb_ptr2];
        int16_t c3 = g_comb_buf3[g_comb_ptr3];

        g_comb_lp0 = (int16_t)div2_trunc(c0 + g_comb_lp0);
        g_comb_lp1 = (int16_t)div2_trunc(c1 + g_comb_lp1);
        g_comb_lp2 = (int16_t)div2_trunc(c2 + g_comb_lp2);
        g_comb_lp3 = (int16_t)div2_trunc(c3 + g_comb_lp3);

        int32_t rev_fb = 22500 + ((preset->reverb_mix * 5500) >> 7);
        int32_t rev_sum = div4_trunc(g_comb_lp0 + g_comb_lp1 + g_comb_lp2 + g_comb_lp3);
        int32_t comb_in0 = mix + q15_mul_trunc(g_comb_lp0, rev_fb);
        int32_t comb_in1 = mix + q15_mul_trunc(g_comb_lp1, rev_fb);
        int32_t comb_in2 = mix + q15_mul_trunc(g_comb_lp2, rev_fb);
        int32_t comb_in3 = mix + q15_mul_trunc(g_comb_lp3, rev_fb);

        if (comb_in0 > 32767)  comb_in0 = 32767;
        if (comb_in0 < -32767) comb_in0 = -32767;
        if (comb_in1 > 32767)  comb_in1 = 32767;
        if (comb_in1 < -32767) comb_in1 = -32767;
        if (comb_in2 > 32767)  comb_in2 = 32767;
        if (comb_in2 < -32767) comb_in2 = -32767;
        if (comb_in3 > 32767)  comb_in3 = 32767;
        if (comb_in3 < -32767) comb_in3 = -32767;

        g_comb_buf0[g_comb_ptr0] = (int16_t)comb_in0;
        g_comb_buf1[g_comb_ptr1] = (int16_t)comb_in1;
        g_comb_buf2[g_comb_ptr2] = (int16_t)comb_in2;
        g_comb_buf3[g_comb_ptr3] = (int16_t)comb_in3;

        if (++g_comb_ptr0 >= 743)  g_comb_ptr0 = 0;
        if (++g_comb_ptr1 >= 887)  g_comb_ptr1 = 0;
        if (++g_comb_ptr2 >= 1013) g_comb_ptr2 = 0;
        if (++g_comb_ptr3 >= 1153) g_comb_ptr3 = 0;

        int32_t rev_wet = (int32_t)preset->reverb_mix + ((int32_t)preset->reverb_mix >> 1);
        mix += (rev_sum * rev_wet) >> 7;

        // Q15 Polynomial Soft-Clip Saturator: x - (x^3 / 4)
        int32_t mix_q15 = mix;
        if (mix_q15 > 32767)  mix_q15 = 32767;
        if (mix_q15 < -32767) mix_q15 = -32767;

        int32_t drive_gain = 1024 + (preset->drive_level * 6);
        int32_t x = (mix_q15 * drive_gain) >> 10;
        if (x > 32767)  x = 32767;
        if (x < -32767) x = -32767;

        int32_t x3 = (x * x) >> 15;
        x3 = (x3 * x) >> 15;
        int32_t sat_out = x - (x3 >> 2);

        // Speaker / Headphone EQ: softens harsh high sizzle while keeping bass/mids punchy
        g_eq_lp += q15_mul_trunc(sat_out - g_eq_lp, 8000);
        int32_t eq_out = g_eq_lp + div2_trunc(sat_out - g_eq_lp);

        buf[i * 2]     += eq_out;
        buf[i * 2 + 1] += eq_out;
    }
}
