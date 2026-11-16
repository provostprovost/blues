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

static BassVoice g_bass;
static KeysVoice g_keys[4];
static int32_t   g_drum_env[4];
static uint16_t  g_drum_decay[4] = { 32000, 31000, 30000, 28000 };
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
    g_eq_lp = 0;
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

void dsp_trigger_drum(uint8_t mask, const uint8_t *drum_vel, uint8_t open_hat) {
    for (int d = 0; d < 4; d++) {
        if (mask & (1 << d)) {
            g_drum_env[d] = (int32_t)drum_vel[d] * 256;
            g_drum_pitch[d] = 0;
            if (d == 2) {
                g_drum_decay[2] = open_hat ? 31800 : 30000;
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

        // Bass Synth: Multi-Harmonic Precision Bass with Pluck Snap & Anti-Click Attack
        if (g_bass.active) {
            g_bass.phase += g_bass.phase_inc;
            int16_t b_fund = hal_sine_q15(g_bass.phase);
            int16_t b_harm = hal_sine_q15(g_bass.phase * 2);
            int16_t b_third = hal_sine_q15(g_bass.phase * 3);

            int32_t b_sample;
            if (g_bass.pluck_samples > 0) {
                g_bass.pluck_samples--;
                // Accentuate 2nd & subtle 3rd harmonic during initial ~50 ms pick bite
                b_sample = (b_fund >> 1) + (b_harm >> 1) + (b_third >> 3);
            } else {
                b_sample = (b_fund * 3) / 4 + (b_harm >> 2);
            }

            int32_t b_amp = (g_bass.env_q15 * preset->bass_vol) >> 15;
            mix += (b_sample * b_amp) >> 8; // Punchy, balanced bass level with clean headroom

            // Attack micro-ramp (smooth 2 ms ramp up to eliminate note-on click/pop)
            if (g_bass.env_q15 < g_bass.target_env) {
                g_bass.env_q15 += (g_bass.target_env - g_bass.env_q15 + 15) >> 4;
            }

            if (g_bass.gate_samples > 0) {
                g_bass.gate_samples--;
                g_bass.env_q15 = (g_bass.env_q15 * 32766) >> 15; // Natural string decay
            } else {
                g_bass.env_q15 = (g_bass.env_q15 * 32650) >> 15; // Smooth release
            }
            if (g_bass.env_q15 < 100) g_bass.active = 0;
        }

        // Keys Synth: 4-voice Polyphony across 4 Distinct Musical Algorithms
        for (int k = 0; k < 4; k++) {
            if (g_keys[k].active) {
                int16_t car = 0;
                int32_t m_env = (g_keys[k].env_q15 * 3) / 4; // Sustained harmonic body

                if (preset->ep_tone_algo == 0) { // Rhodes FM: sweet bell tine attack with warm sustained EP body
                    g_keys[k].phase_c += g_keys[k].inc_c;
                    g_keys[k].phase_m += g_keys[k].inc_c; // 1:1 modulator
                    int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                    uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 16000);
                    car = hal_sine_q15(g_keys[k].phase_c + pmod);
                } else if (preset->ep_tone_algo == 1) { // Bright FM EP: crisp 2:1 chime attack with rich overtones
                    g_keys[k].phase_c += g_keys[k].inc_c;
                    g_keys[k].phase_m += g_keys[k].inc_c * 2; // 2:1 modulator
                    int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                    uint32_t pmod = (uint32_t)((((int32_t)mod * m_env) >> 15) * 24000);
                    car = hal_sine_q15(g_keys[k].phase_c + pmod);
                } else if (preset->ep_tone_algo == 2) { // B3 Drawbar Organ: cutting harmonic drawbars
                    g_keys[k].phase_c += g_keys[k].inc_c;
                    int16_t h1 = hal_sine_q15(g_keys[k].phase_c);
                    int16_t h2 = hal_sine_q15(g_keys[k].phase_c * 2);
                    int16_t h3 = hal_sine_q15(g_keys[k].phase_c * 3);
                    int16_t h4 = hal_sine_q15(g_keys[k].phase_c * 4);
                    car = (h1 >> 2) + (h2 >> 1) + (h3 >> 2) + (h4 >> 3);
                } else { // Clavinet: biting 3:1 funk pluck
                    g_keys[k].phase_c += g_keys[k].inc_c;
                    g_keys[k].phase_m += g_keys[k].inc_c * 3;
                    int16_t mod = hal_sine_q15(g_keys[k].phase_m);
                    uint32_t pmod = (uint32_t)((((int32_t)mod * g_keys[k].env_q15) >> 15) * 28000);
                    car = hal_sine_q15(g_keys[k].phase_c + pmod);
                }

                int32_t k_amp = (g_keys[k].env_q15 * preset->keys_vol) >> 15;
                mix += (car * k_amp) >> 8; // Prominent, articulate keys level

                if (g_keys[k].gate_samples > 0) {
                    g_keys[k].gate_samples--;
                    if (preset->ep_tone_algo != 2) {
                        g_keys[k].env_q15 = (g_keys[k].env_q15 * 32766) >> 15;
                    }
                } else {
                    g_keys[k].env_q15 = (g_keys[k].env_q15 * 32650) >> 15;
                }
                if (g_keys[k].env_q15 < 100) g_keys[k].active = 0;
            }
        }

        // Drums: Kick (0)
        if (g_drum_env[0] > 0) {
            uint32_t kick_inc = 4380000u + (uint32_t)(g_drum_env[0] * 260);
            g_drum_pitch[0] += kick_inc;
            int16_t k_sample = hal_sine_q15(g_drum_pitch[0]);
            int32_t d0_amp = (g_drum_env[0] * preset->drums_vol) >> 15;
            mix += (k_sample * d0_amp) >> 8;
            g_drum_env[0] = (g_drum_env[0] * g_drum_decay[0]) >> 15;
        }

        // Drums: Snare (1)
        if (g_drum_env[1] > 0) {
            g_lfsr = (g_lfsr >> 1) ^ (-(g_lfsr & 1u) & 0xB400u);
            int16_t noise_sample = (int16_t)(g_lfsr & 0x7FFF);
            g_drum_pitch[1] += 17540000u;
            int16_t body_sample = hal_sine_q15(g_drum_pitch[1]);
            int16_t s_sample = (noise_sample >> 1) + (body_sample >> 1);
            int32_t d1_amp = (g_drum_env[1] * preset->drums_vol) >> 15;
            mix += (s_sample * d1_amp) >> 9;
            g_drum_env[1] = (g_drum_env[1] * g_drum_decay[1]) >> 15;
        }

        // Drums: Hi-Hat (2)
        if (g_drum_env[2] > 0) {
            g_lfsr = (g_lfsr >> 1) ^ (-(g_lfsr & 1u) & 0xB400u);
            int16_t n_sample = (int16_t)(g_lfsr & 0xFFFF);
            int32_t d2_amp = (g_drum_env[2] * preset->drums_vol) >> 15;
            mix += (n_sample * d2_amp) >> 10;
            g_drum_env[2] = (g_drum_env[2] * g_drum_decay[2]) >> 15;
        }

        // Drums: Rimshot (3)
        if (g_drum_env[3] > 0) {
            g_drum_pitch[3] += 12000000u;
            int16_t r_sample = hal_sine_q15(g_drum_pitch[3]);
            int32_t d3_amp = (g_drum_env[3] * preset->drums_vol) >> 15;
            mix += (r_sample * d3_amp) >> 8;
            g_drum_env[3] = (g_drum_env[3] * g_drum_decay[3]) >> 15;
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
