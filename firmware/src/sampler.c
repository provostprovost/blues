/* SPDX-License-Identifier: GPL-3.0-only */
/* Blues Box 2.3: the sample player behind the "real" voices (bass, drums, tenor sax, guitar). Included by
 * dsp_voices.c. The samples are block ADPCM in firmware/gen/sample_bank.h (tools/build_samples.py), decoded as they
 * play with Erik37's codec (erk_codec.h, MPL-2.0); a voice reads its zone at any rate (the pitch, a glide, a
 * vibrato) with linear interpolation, and loops where the bank says so. */
#include "erk_codec.h"

typedef struct {
    uint32_t off;           /* byte offset of the zone's first block in SMP_DATA */
    uint32_t len;           /* samples */
    uint32_t loop;          /* loop start (a multiple of 32), samples */
    uint32_t loop_len;      /* 0: a one-shot */
    int16_t  h1, h2;        /* the decoder's history at the loop start */
    uint32_t k;             /* a note: step (Q16) = ((phase increment >> 8) * k) >> 16; a one-shot: the step itself */
    uint16_t gain;          /* Q12 */
    uint8_t  root, hi;      /* the zone's own key; the highest key it plays */
} SmpZone;

#include "../gen/sample_bank.h"

typedef struct {
    erk_dec d;
    const SmpZone *z;
    uint32_t pos;           /* source samples decoded so far */
    uint32_t frac;          /* Q16 */
    int32_t  s0, s1;
    uint8_t  on;
} SmpVoice;

/* the zone of a key among an instrument's n zones (they are sorted by key) */
static const SmpZone *smp_zone(uint32_t first, uint32_t n, uint8_t key) {
    for (uint32_t i = 0; i + 1u < n; i++) {
        if (key <= SMP_ZONES[first + i].hi) return &SMP_ZONES[first + i];
    }
    return &SMP_ZONES[first + n - 1u];
}

static void smp_start(SmpVoice *v, const SmpZone *z) {
    v->z = z;
    erk_dec_start(&v->d, SMP_DATA + z->off, 0, 0);
    v->frac = 0;
    v->s0 = 0;
    v->s1 = erk_dec_next(&v->d);
    v->pos = 1;
    v->on = 1;
}

/* the step of a note played with this phase increment (G_MIDI_PHASE_INC and anything bent from it). Its 64-bit
 * product stays in a function of its own, away from the per-sample loops (as Erik37 does: on the AC79 a 32 x 32 -> 64
 * multiply beside a load can leave a register off). */
static __attribute__((noinline)) uint32_t smp_step(const SmpZone *z, uint32_t phase_inc) {
    return (uint32_t)(((uint64_t)(phase_inc >> 8) * z->k) >> 16);
}

/* the voice's next output sample (16 bit) */
static inline int32_t smp_next(SmpVoice *v, uint32_t step_q16) {
    if (!v->on) return 0;
    v->frac += step_q16;
    while (v->frac >= 65536u) {
        const SmpZone *z = v->z;
        v->frac -= 65536u;
        v->s0 = v->s1;
        if (z->loop_len && v->pos >= z->loop + z->loop_len) {
            erk_dec_start(&v->d, SMP_DATA + z->off + (z->loop / 32u) * 17u, z->h1, z->h2);
            v->pos = z->loop;
        }
        if (v->pos >= z->len) {
            v->on = 0;
            v->s1 = 0;
            break;
        }
        v->s1 = erk_dec_next(&v->d);
        v->pos++;
    }
    return v->s0 + (((v->s1 - v->s0) * (int32_t)(v->frac >> 1)) >> 15);
}
