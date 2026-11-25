/* SPDX-License-Identifier: GPL-3.0-only */
/* Blues Box 2.3: the lead's pedalboard. Two slots in a row (FX1 into FX2), one knob each (0..100), the lead voice
 * only. Fixed point, one sample at a time; included by dsp_voices.c.
 *
 * Each slot has a delay line of its own (0.74 s) that whichever effect sits in the slot uses its own way: an echo,
 * a chorus' short modulated tap, the combs of the spring reverb. Changing the effect clears the line. */

#if defined(DINGHY_HOST) || defined(STANDALONE_TEST)
#define LFX_POOL
#else
#define LFX_POOL __attribute__((section(".pool")))
#endif

#define LFX_N    32768u
#define LFX_MASK (LFX_N - 1u)
#define LFX_HZ(f_milli) ((uint32_t)(f_milli) * 97u + ((uint32_t)(f_milli) * 2u) / 5u)   /* phase step of f_milli / 1000 Hz (2^32 / 44.1e6 = 97.39) */

typedef struct {
    uint8_t  type;
    uint32_t w;             /* the line's write position */
    uint32_t lfo, lfo2;
    uint32_t n;
    int32_t  a[12];         /* filter / pointer state, per effect */
} LfxSlot;

static int16_t g_lfx_line[2][LFX_N] LFX_POOL;
static LfxSlot g_lfx[2];
static uint32_t g_lfx_quiet = 0;        /* samples since the lead last sounded: the board sleeps when its tails are over */

static inline int32_t lfx_clip15(int32_t v) {
    return v > 32767 ? 32767 : v < -32767 ? -32767 : v;
}

/* the line, delay_q8 / 256 samples ago (linear interpolation) */
static inline int32_t lfx_tap(const int16_t *line, uint32_t w, uint32_t delay_q8) {
    uint32_t d = delay_q8 >> 8;
    int32_t f = (int32_t)(delay_q8 & 255u);
    int32_t s0 = line[(w - d) & LFX_MASK], s1 = line[(w - d - 1u) & LFX_MASK];
    return s0 + (((s1 - s0) * f) >> 8);
}

static void lfx_reset(uint8_t slot, uint8_t type) {
    LfxSlot *s = &g_lfx[slot];
    for (uint32_t i = 0; i < LFX_N; i++) g_lfx_line[slot][i] = 0;
    for (int i = 0; i < 12; i++) s->a[i] = 0;
    s->w = 0; s->lfo = 0; s->lfo2 = 0; s->n = 0;
    s->type = type;
}

static void lfx_init(void) {
    lfx_reset(0, LFX_OFF);
    lfx_reset(1, LFX_OFF);
    g_lfx_quiet = 0;
}

/* a comb of the spring reverb: its own stretch of the line, a damped feedback */
static inline int32_t lfx_comb(int16_t *line, int32_t *ptr, int32_t *lp, uint32_t base, uint32_t len, int32_t in, int32_t fb) {
    int32_t out = line[base + (uint32_t)*ptr];
    *lp += ((out - *lp) * 20000) >> 15;
    line[base + (uint32_t)*ptr] = (int16_t)lfx_clip15(in + ((*lp * fb) >> 15));
    if (++*ptr >= (int32_t)len) *ptr = 0;
    return out;
}

static inline int32_t lfx_allpass(int16_t *line, int32_t *ptr, uint32_t base, uint32_t len, int32_t in) {
    int32_t buf = line[base + (uint32_t)*ptr];
    int32_t out = buf - (in >> 1);
    line[base + (uint32_t)*ptr] = (int16_t)lfx_clip15(in + (out >> 1));
    if (++*ptr >= (int32_t)len) *ptr = 0;
    return out;
}

static int32_t lfx_process(uint8_t slot, int32_t x, uint8_t amt, uint8_t bpm) {
    LfxSlot *s = &g_lfx[slot];
    int16_t *line = g_lfx_line[slot];
    int32_t am = (amt > 100) ? 100 : amt;

    switch (s->type) {
        case LFX_OVERDRIVE: {
            /* a tube screamer's shape: gain into a smooth clip, the top rolled off */
            int32_t v = lfx_clip15((x * (256 + am * 24)) >> 8);
            int32_t v3 = (((v * v) >> 15) * v) >> 15;
            int32_t y = v + ((v - v3) >> 1);
            s->a[0] += ((y - s->a[0]) * 14000) >> 15;
            return (s->a[0] * (24 - am / 12)) >> 5;
        }
        case LFX_FUZZ: {
            /* a lot of gain into hard, lopsided clipping */
            int32_t v = (x * (1024 + am * 200)) >> 8;
            if (v > 20000) v = 20000;
            else if (v < -14000) v = -14000;
            s->a[0] += ((v - s->a[0]) * 9000) >> 15;
            return (s->a[0] * 29) >> 5;
        }
        case LFX_COMPRESSOR: {
            /* evens the notes out: quick down, slow back up, then make-up gain */
            int32_t ax = x < 0 ? -x : x;
            if (ax > s->a[0]) s->a[0] += (ax - s->a[0]) >> 5;
            else s->a[0] -= s->a[0] >> 12;
            if ((s->n++ & 15u) == 0) {
                int32_t g = 4096;
                if (s->a[0] > 5000) g = (5000 * 4096) / s->a[0];
                g = 4096 - ((4096 - g) * am) / 100;
                s->a[1] = (g * (4096 + am * 50)) >> 12;
            }
            s->a[2] += (s->a[1] - s->a[2]) >> 6;
            return (x * s->a[2]) >> 12;
        }
        case LFX_CHORUS: {
            s->lfo += LFX_HZ(800);
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(x);
            int32_t d = 529 * 256 + ((hal_sine_q15(s->lfo) * (60 + am * 2)) >> 7);
            int32_t wet = lfx_tap(line, s->w, (uint32_t)d);
            s->w++;
            return (x * 200 + wet * (40 + am)) >> 8;
        }
        case LFX_FLANGER: {
            s->lfo += LFX_HZ(300);
            int32_t d = 70 * 256 + ((hal_sine_q15(s->lfo) * (20 + am / 2)) >> 7);
            int32_t wet = lfx_tap(line, s->w, (uint32_t)d);
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(x + ((wet * (8000 + am * 130)) >> 15));
            s->w++;
            return ((x + wet) * 23) >> 5;
        }
        case LFX_PHASER:
        case LFX_UNIVIBE: {
            /* four all-pass stages swept together (the phaser) or each at its own depth (the vibe's uneven throb) */
            uint8_t vibe = (s->type == LFX_UNIVIBE);
            s->lfo += vibe ? LFX_HZ(1500 + am * 40) : LFX_HZ(400 + am * 12);
            int32_t sweep = hal_sine_q15(s->lfo);
            int32_t in = x + ((s->a[8] * (vibe ? 0 : 9000)) >> 15);
            int32_t y = in;
            for (int st = 0; st < 4; st++) {
                int32_t c = 18000 + (((sweep * (vibe ? (5000 + st * 1800) : 10500)) >> 15));
                int32_t out = ((c * (y - s->a[st * 2 + 1])) >> 15) + s->a[st * 2];
                s->a[st * 2] = y;
                s->a[st * 2 + 1] = out;
                y = out;
            }
            s->a[8] = y;
            return vibe ? ((x + y) * 22) >> 5 : (x * 180 + y * (80 + am)) >> 8;
        }
        case LFX_TREMOLO: {
            s->lfo += LFX_HZ(2500 + am * 60);
            int32_t depth = 9000 + am * 200;
            int32_t m = 32767 - ((depth * (hal_sine_q15(s->lfo) + 32768)) >> 16);
            return (x * m) >> 15;
        }
        case LFX_VIBRATO: {
            s->lfo += LFX_HZ(5500);
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(x);
            int32_t d = 180 * 256 + ((hal_sine_q15(s->lfo) * (10 + am)) >> 7);
            int32_t wet = lfx_tap(line, s->w, (uint32_t)d);
            s->w++;
            return wet;
        }
        case LFX_ROTARY: {
            /* a rotating speaker: the horn (Doppler and loudness) over the slower bass rotor; the knob's lower half
             * is the slow speed, its upper half the fast one, and the rotors take their time getting there */
            int32_t target = (am >= 50) ? (int32_t)LFX_HZ(6600) : (int32_t)LFX_HZ(800);
            if (s->a[0] == 0) s->a[0] = target;
            s->a[0] += (target - s->a[0]) >> 15;
            s->a[0] += (target > s->a[0]) ? 1 : (target < s->a[0]) ? -1 : 0;
            s->lfo += (uint32_t)s->a[0];
            s->lfo2 += (uint32_t)(s->a[0] - (s->a[0] >> 3));
            s->a[1] += ((x - s->a[1]) * 3600) >> 15;
            int32_t lo = s->a[1], hi = x - lo;
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(hi);
            int32_t d = 40 * 256 + ((hal_sine_q15(s->lfo) * 18) >> 7);
            int32_t horn = lfx_tap(line, s->w, (uint32_t)d);
            s->w++;
            horn = (horn * (24576 + (hal_sine_q15(s->lfo + 0x40000000u) >> 2))) >> 15;
            lo = (lo * (27852 + ((hal_sine_q15(s->lfo2) * 4915) >> 15))) >> 15;
            return horn + lo;
        }
        case LFX_AUTOWAH: {
            /* an envelope filter: the harder the note, the further the band-pass opens */
            int32_t ax = x < 0 ? -x : x;
            if (ax > s->a[0]) s->a[0] += (ax - s->a[0]) >> 7;
            else s->a[0] -= s->a[0] >> 12;
            int32_t f = 1400 + ((s->a[0] * (2300 + am * 100)) >> 15);
            if (f > 10500) f = 10500;
            s->a[1] += (f * s->a[2]) >> 15;
            int32_t high = x - s->a[1] - (s->a[2] >> 2);
            s->a[2] += (f * high) >> 15;
            s->a[1] = lfx_clip15(s->a[1]);
            s->a[2] = lfx_clip15(s->a[2]);
            return (s->a[2] * (128 + am) + x * (128 - am)) >> 8;
        }
        case LFX_SLAPBACK: {
            int32_t wet = line[(s->w - 4851u) & LFX_MASK];
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(x + (wet >> 2));
            s->w++;
            return x + (wet * (30 + am)) / 200;
        }
        case LFX_DELAY: {
            /* an echo in time with the band: a quarter note (an eighth where a quarter is longer than the line) */
            uint32_t d = 2646000u / (bpm < 40 ? 112u : bpm);
            while (d > LFX_N - 64u) d >>= 1;
            int32_t wet = line[(s->w - d) & LFX_MASK];
            s->a[0] += ((wet - s->a[0]) * 16000) >> 15;
            line[s->w & LFX_MASK] = (int16_t)lfx_clip15(x + ((s->a[0] * (8200 + am * 130)) >> 15));
            s->w++;
            return x + (wet * (20 + am)) / 170;
        }
        case LFX_SPRING_REVERB: {
            int32_t in = x >> 2;
            int32_t c = lfx_comb(line, &s->a[0], &s->a[6], 0u, 1423u, in, 27500)
                      + lfx_comb(line, &s->a[1], &s->a[7], 2048u, 1667u, in, 27200)
                      + lfx_comb(line, &s->a[2], &s->a[8], 4096u, 1877u, in, 26900)
                      + lfx_comb(line, &s->a[3], &s->a[9], 8192u, 2131u, in, 26600);
            c = lfx_allpass(line, &s->a[4], 12288u, 487u, c >> 1);
            c = lfx_allpass(line, &s->a[5], 13312u, 163u, c);
            return x + (c * (20 + am)) / 70;
        }
        case LFX_LOFI: {
            /* an old sampler: fewer samples a second, fewer bits */
            uint32_t hold = 1u + (uint32_t)am / 9u;
            if (s->n++ % hold == 0) {
                int sh = 5 + am / 20;
                s->a[0] = (x >> sh) << sh;
            }
            return s->a[0];
        }
        default:
            return x;
    }
}

/* the lead's sample through FX1 then FX2 */
static int32_t lfx_run(int32_t x, const PresetConfig *preset, uint8_t lead_active) {
    uint8_t t1 = preset->lead_fx1 % NUM_LEAD_FX, t2 = preset->lead_fx2 % NUM_LEAD_FX;
    if (t1 == LFX_OFF && t2 == LFX_OFF) return x;
    if (lead_active || x != 0) g_lfx_quiet = 0;
    else if (g_lfx_quiet < 0x7FFFFFFFu) g_lfx_quiet++;
    if (g_lfx[0].type != t1) lfx_reset(0, t1);
    if (g_lfx[1].type != t2) lfx_reset(1, t2);
    if (g_lfx_quiet > 6u * 44100u) return 0;        /* every tail is over */
    if (t1 != LFX_OFF) x = lfx_process(0, x, preset->lead_fx1_amt, preset->bpm);
    if (t2 != LFX_OFF) x = lfx_process(1, x, preset->lead_fx2_amt, preset->bpm);
    return x;
}
