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
    int32_t  env_q15;
    int32_t  target_env;
    uint32_t gate_samples;
    uint8_t  active;
    uint8_t  midi_note;
    int32_t  filter_lp;
    uint32_t vib_phase;
    uint16_t vib_delay;
} LeadVoice;

static BassVoice g_bass;
static KeysVoice g_keys[4];
static LeadVoice g_lead[MAX_LEAD_VOICES];
static int32_t   g_bass_lp = 0;
static int32_t   g_drum_env[4];
static uint16_t  g_drum_decay[4] = { 32758, 32745, 32720, 32725 };
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
        g_lead[v].env_q15 = 0;
        g_lead[v].gate_samples = 0;
        g_lead[v].filter_lp = 0;
        g_lead[v].vib_phase = 0;
        g_lead[v].vib_delay = 0;
        g_lead[v].midi_note = 0xFF;
    }
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
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        g_lead[v].active = 0;
        g_lead[v].env_q15 = 0;
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
    g_bass.gate_samples = 16000; // ~360 ms walking bass note sustain
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

void dsp_note_on_lead(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode) {
    int target_v = -1;
    // 1. Check if this exact note is already sounding
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        if (g_lead[v].active && g_lead[v].midi_note == midi_note) {
            target_v = v;
            break;
        }
    }
    // 2. Otherwise find an inactive voice slot
    if (target_v < 0) {
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (!g_lead[v].active) {
                target_v = v;
                break;
            }
        }
    }
    // 3. If all voices active, steal the quietest voice
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
    lv->phase_inc = get_scaled_inc(midi_note, tuning_mode);
    lv->midi_note = midi_note;
    lv->target_env = (int32_t)velocity * 256;
    if (!lv->active) {
        lv->phase = 0;
        lv->phase2 = 0;
        lv->env_q15 = 0;
        lv->vib_delay = 1800; // ~40 ms delay before expressive blues vibrato onset
        lv->vib_phase = 0;
        lv->filter_lp = 0;
    } else {
        // Legato tie
        lv->env_q15 = (lv->env_q15 * 3 + lv->target_env) / 4;
        lv->vib_delay = 1200;
    }
    lv->gate_samples = 44100 * 10;
    lv->active = 1;
}

void dsp_note_off_lead(uint8_t midi_note) {
    for (int v = 0; v < MAX_LEAD_VOICES; v++) {
        if (g_lead[v].active) {
            if (midi_note == 0xFF || g_lead[v].midi_note == midi_note) {
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

            if (preset->bass_instrument == BASS_UPRIGHT) {
                // Upright Double Bass: woody fundamental + body slap transient
                b_sample = (b_fund * 3 / 4) + (b_harm >> 2);
                if (g_bass.pluck_samples > 0) {
                    g_bass.pluck_samples--;
                    b_sample += (int32_t)g_bass.pluck_samples * 6;
                }
            } else if (preset->bass_instrument == BASS_MUTED_TICTAC) {
                // Muted Tic-Tac Boogie: tight palm-mute bite with heavy 2nd harmonic
                if (g_bass.pluck_samples > 0) g_bass.pluck_samples--;
                b_sample = (b_fund >> 2) + (b_harm >> 1) + (b_third >> 2);
            } else if (preset->bass_instrument == BASS_MOOG_FUNK) {
                // Moog Funk: rich analog saw with dynamic envelope filter sweep
                int16_t b_fourth = hal_sine_q15(g_bass.phase * 4);
                int32_t saw = (b_fund >> 1) + (b_harm >> 2) + (b_third >> 3) + (b_fourth >> 4);
                int32_t cutoff = 3000 + ((g_bass.env_q15 * 24000) >> 15);
                g_bass_lp += ((saw - g_bass_lp) * cutoff) >> 15;
                b_sample = g_bass_lp;
                if (g_bass.pluck_samples > 0) g_bass.pluck_samples--;
            } else {
                // Fender P-Bass (Default): punchy roundwound with initial pluck bite
                if (g_bass.pluck_samples > 0) {
                    g_bass.pluck_samples--;
                    b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 2);
                } else {
                    b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 3);
                }
            }

            int32_t b_amp = (g_bass.env_q15 * preset->bass_vol) >> 15;
            mix += (b_sample * b_amp) >> 7; // Driving, punchy walking bass

            // Attack micro-ramp
            if (g_bass.env_q15 < g_bass.target_env) {
                g_bass.env_q15 += (g_bass.target_env - g_bass.env_q15 + 15) >> 4;
            }

            if (g_bass.gate_samples > 0) {
                g_bass.gate_samples--;
                if (preset->bass_instrument == BASS_MUTED_TICTAC) {
                    g_bass.env_q15 = (g_bass.env_q15 * 32300) >> 15; // Tight palm-mute decay
                } else {
                    g_bass.env_q15 = (g_bass.env_q15 * 32766) >> 15; // Natural string decay
                }
            } else {
                g_bass.env_q15 = (g_bass.env_q15 * 32650) >> 15;
            }
            if (g_bass.env_q15 < 100) g_bass.active = 0;
        }

        // 2. Keys Synth Engine: 4-Voice Polyphony across 8 Instruments
        for (int k = 0; k < 4; k++) {
            if (g_keys[k].active) {
                int16_t car = 0;
                int32_t m_env = (g_keys[k].env_q15 * 3) / 4;

                switch (preset->ep_tone_algo % 8) {
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
                    case KEYS_LESLIE_ORGAN: // 11: Rotary Leslie Swell Organ
                    default: {
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
                }

                int32_t k_amp = (g_keys[k].env_q15 * preset->keys_vol) >> 15;
                mix += (car * k_amp) >> 8;

                if (g_keys[k].gate_samples > 0) {
                    g_keys[k].gate_samples--;
                    if (preset->ep_tone_algo != KEYS_B3_CLEAN && preset->ep_tone_algo != KEYS_B3_DIRTY && preset->ep_tone_algo != KEYS_LESLIE_ORGAN) {
                        g_keys[k].env_q15 = (g_keys[k].env_q15 * 32766) >> 15;
                    }
                } else {
                    g_keys[k].env_q15 = (g_keys[k].env_q15 * 32650) >> 15;
                }
                if (g_keys[k].env_q15 < 100) g_keys[k].active = 0;
            }
        }

        // 3. Synthesized Drums (4 Selectable Kits: Studio, Brush, Retro 808, Stax Tight)
        // Kick (0)
        if (g_drum_env[0] > 0) {
            uint32_t kick_inc;
            if (preset->drum_kit == DRUM_BRUSH) {
                kick_inc = 3200000u + (uint32_t)(g_drum_env[0] * 180);
            } else if (preset->drum_kit == DRUM_RETRO_808) {
                kick_inc = 2200000u + (uint32_t)(g_drum_env[0] * 320);
            } else if (preset->drum_kit == DRUM_STAX_TIGHT) {
                kick_inc = 5100000u + (uint32_t)(g_drum_env[0] * 300);
            } else {
                kick_inc = 4380000u + (uint32_t)(g_drum_env[0] * 260);
            }
            g_drum_pitch[0] += kick_inc;
            int16_t k_sample = hal_sine_q15(g_drum_pitch[0]);
            int32_t d0_amp = (g_drum_env[0] * preset->drums_vol) >> 15;
            mix += (k_sample * d0_amp) >> 7;
            g_drum_env[0] = (g_drum_env[0] * g_drum_decay[0]) >> 15;
        }

        // Snare (1)
        if (g_drum_env[1] > 0) {
            g_lfsr = (g_lfsr >> 1) ^ (-(g_lfsr & 1u) & 0xB400u);
            int16_t noise_sample = (int16_t)(g_lfsr & 0x7FFF);
            int16_t s_sample;
            if (preset->drum_kit == DRUM_BRUSH) {
                s_sample = noise_sample >> 1; // Soft brush swirl
            } else if (preset->drum_kit == DRUM_RETRO_808) {
                g_drum_pitch[1] += 22000000u;
                int16_t body = hal_sine_q15(g_drum_pitch[1]);
                s_sample = (noise_sample * 3 / 4) + (body >> 2); // Snappy 808 clap/snare
            } else if (preset->drum_kit == DRUM_STAX_TIGHT) {
                g_drum_pitch[1] += 19500000u;
                int16_t body = hal_sine_q15(g_drum_pitch[1]);
                s_sample = (noise_sample * 5 / 8) + (body >> 1); // Dampened Stax crack
            } else {
                g_drum_pitch[1] += 17540000u;
                int16_t body = hal_sine_q15(g_drum_pitch[1]);
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
            mix += (n_sample * d2_amp) >> 8;
            g_drum_env[2] = (g_drum_env[2] * g_drum_decay[2]) >> 15;
        }

        // Rimshot (3)
        if (g_drum_env[3] > 0) {
            uint32_t rim_inc = (preset->drum_kit == DRUM_RETRO_808) ? 16000000u :
                               ((preset->drum_kit == DRUM_BRUSH) ? 8500000u : 12000000u);
            g_drum_pitch[3] += rim_inc;
            int16_t r_sample = hal_sine_q15(g_drum_pitch[3]);
            int32_t d3_amp = (g_drum_env[3] * preset->drums_vol) >> 15;
            mix += (r_sample * d3_amp) >> 7;
            g_drum_env[3] = (g_drum_env[3] * g_drum_decay[3]) >> 15;
        }

        // 4. Solo Lead Voice (4-Voice Polyphony across 4 Lead Instruments)
        for (int v = 0; v < MAX_LEAD_VOICES; v++) {
            if (g_lead[v].active) {
                uint32_t cur_inc = g_lead[v].phase_inc;
                if (g_lead[v].vib_delay > 0) {
                    g_lead[v].vib_delay--;
                } else {
                    g_lead[v].vib_phase += 520000u; // ~5.3 Hz expressive blues vibrato
                    int16_t vib = hal_sine_q15(g_lead[v].vib_phase);
                    int32_t delta = (int32_t)((g_lead[v].phase_inc >> 10) * vib) >> 11;
                    cur_inc = (uint32_t)((int32_t)g_lead[v].phase_inc + delta);
                }
                g_lead[v].phase += cur_inc;

                int32_t lead_sample = 0;
                switch (preset->lead_instrument % 8) {
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
                    case LEAD_PERC_ORGAN: // 7: Percussive B3 Organ Lead
                    default: {
                        int16_t o1 = hal_sine_q15(g_lead[v].phase);
                        int16_t o2 = hal_sine_q15(g_lead[v].phase * 2);
                        int16_t o3 = hal_sine_q15(g_lead[v].phase * 3);
                        lead_sample = (o1 >> 1) + (o2 >> 2) + (o3 * 3 / 8);
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

        int32_t rev_sum = div4_trunc(g_comb_lp0 + g_comb_lp1 + g_comb_lp2 + g_comb_lp3);
        int32_t comb_in0 = mix + q15_mul_trunc(g_comb_lp0, 22500);
        int32_t comb_in1 = mix + q15_mul_trunc(g_comb_lp1, 22500);
        int32_t comb_in2 = mix + q15_mul_trunc(g_comb_lp2, 22500);
        int32_t comb_in3 = mix + q15_mul_trunc(g_comb_lp3, 22500);

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

        mix += (rev_sum * preset->reverb_mix) >> 7;

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
