/* SPDX-License-Identifier: GPL-3.0-only */
/* Blues Box 2.3 pattern engine: drum beats with their own kick / snare / hat / ride lanes, fills, and riff-based
 * bass lines. Included by blues_arranger.c (one translation unit with it).
 *
 * Everything is written as text, one character a step, so a pattern can be read and edited like a drum chart:
 *   grid 12: three steps a beat (triplets; a shuffle is steps 1 and 3 of each beat)
 *   grid 16: four steps a beat (16ths; the swing knob moves the off-beat steps)
 * Drum lanes:  X accent, x hit, g ghost / soft, . nothing; hat also o open, p pedal; snare also r cross-stick;
 *              ride also b bell; aux: h high tom, l low tom
 * Bass riffs:  1 root, 2, m (b3), 3 (the chord's third), 4, d (b5), 5, 6, 7 (the chord's 7th), 8 octave,
 *              V S Z the 5th / 6th / b7 below the root, x dead note,
 *              a / A a half step below / above the next chord's root, w a whole step below it,
 *              n a half step from the side the line is coming from,
 *              - hold the note, . rest */

#define DPF_RIDE_DEFAULT 1u     /* the ride lane is the beat's own top line (jazz); the hat keeps time with the foot */
#define DPF_NO_RIDE      2u     /* never moves to the ride */
#define DPF_FIXED_12_8   4u     /* a 12/8 beat: even triplets, whatever the swing knob says */

typedef struct {
    uint8_t grid, bars, flags;
    const char *kick, *snare, *hat, *ride, *aux;      /* grid * bars characters each; 0: an empty lane */
} DrumPat;

static const DrumPat DRUM_PATS[NUM_GROOVES] = {
    /* 0 SLOW 12/8: the slow blues. Triplets on the hat, a lazy kick, the backbeat; the ride for later choruses */
    { 12, 2, DPF_FIXED_12_8,
      "x....xx....." "x....xx....x",
      "...X.....X.." "...X.....X.g",
      "XxxXxxXxxXxx" "XxxXxxXxxXxx",
      "XxxXxxXxxXxx" "XxxXxxXxxXxx", 0 },
    /* 1 TEXAS SHUFFLE: the double shuffle. The snare shuffles along with the hat, cracking on 2 and 4 */
    { 12, 2, 0,
      "x..x..x..x.." "x..x..x..x.x",
      "g.gX.gg.gX.g" "g.gX.gg.gX.g",
      "x.xx.xx.xx.x" "x.xx.xx.xx.x",
      "x.xX.xx.xX.x" "x.xX.xx.xX.x", 0 },
    /* 2 CHICAGO SHUFFLE: the downbeats lead, kick on 1 and 3, a plain fat backbeat */
    { 12, 2, 0,
      "x.....x....." "x.....x....x",
      "...X.....X.." "...X.....X..",
      "X.xX.xX.xX.x" "X.xX.xX.xX.x",
      "X.xX.xX.xX.x" "X.xX.xX.xX.x", 0 },
    /* 3 ROCKIN' BLUES: straight 8ths */
    { 16, 2, 0,
      "x.......x.x....." "x.....x.x.x.....",
      "....X.......X..." "....X.......X...",
      "X.x.X.x.X.x.X.x." "X.x.X.x.X.x.X.o.",
      "X.x.X.x.X.x.X.x." "X.x.X.x.X.x.X.x.", 0 },
    /* 4 NEW ORLEANS 2ND LINE: the snare carries the street beat, the bass drum answers */
    { 16, 2, DPF_NO_RIDE,
      "x.....x.....x..." "x.....x...x.x...",
      "g.xg.gx.g.xg.gX." "g.xg.gx.gxg.X.X.",
      "....p.......p..." "....p.......p...",
      0, 0 },
    /* 5 MEMPHIS SOUL: dry, even, nothing extra */
    { 16, 2, 0,
      "x.......x.x....." "x......xx.x.....",
      "....X.......X..." "....X.......X...",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.o.",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.", 0 },
    /* 6 RHUMBA BLUES: snares off. Tresillo kick, cross-stick on 2, toms answering on 4 */
    { 16, 2, 0,
      "x..x..x.x..x..x." "x..x..x.x..x..x.",
      "....r.......r..." "....r..........r",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.",
      "b.x.b.x.b.x.b.x." "b.x.b.x.b.x.b.x.",
      "..........h...l." "..........h.l.l." },
    /* 7 BOOGIE-WOOGIE: four on the floor under a fast shuffle */
    { 12, 2, 0,
      "x..x..x..x.." "x..x..x..x..",
      "...X.....X.." "...X.....X.g",
      "x.xx.xx.xx.x" "x.xx.xx.xx.x",
      "x.xX.xx.xX.x" "x.xX.xx.xX.x", 0 },
    /* 8 PURDIE SHUFFLE: half time. The backbeat on 3, ghost notes on the middle triplets */
    { 12, 2, 0,
      "x....x......" "x....x.....x",
      ".g..g.Xg..g." ".g..g.Xg..g.",
      "x.xx.xx.xx.x" "x.xx.xx.xx.o",
      "x.xx.xx.xx.x" "x.xx.xx.xx.x", 0 },
    /* 9 TRAIN BEAT: 16ths on the snare, the accents on the "and"s, four on the floor */
    { 16, 1, DPF_NO_RIDE,
      "x...x...x...x...",
      "ggXgggXgggXgggXg",
      "....p.......p...",
      0, 0 },
    /* 10 BO DIDDLEY: the 3-2 clave on the toms, maracas on the hat */
    { 16, 1, DPF_NO_RIDE,
      "x.......x.......",
      0,
      "gxgxgxgxgxgxgxgx",
      0,
      "l..l..l...h.h..." },
    /* 11 JAZZ SHUFFLE: the ride's skip beat, a feathered kick, the hat with the foot, cross-stick comping */
    { 12, 2, DPF_RIDE_DEFAULT,
      "g..g..g..g.." "g..g..g..g..",
      "...r.....r.." ".....g...r.g",
      "...p.....p.." "...p.....p..",
      "x..x.xx..x.x" "x..x.xx..x.x", 0 },
    /* 12 GOSPEL SWING: big 1 and 3, the push into the next bar */
    { 12, 2, 0,
      "x.....x....x" "x....xx....x",
      "...X.....X.." "...X.....X..",
      "X.xX.xX.xX.x" "X.xX.xX.xX.o",
      "X.xX.xX.xX.x" "X.xX.xX.xX.x", 0 },
    /* 13 DELTA STOMP: a foot and a hand, nothing else */
    { 12, 1, DPF_NO_RIDE,
      "X..x..X..x..",
      "...X.....X..",
      0, 0, 0 },
    /* 14 70s POCKET FUNK */
    { 16, 2, 0,
      "x......x..x....." "x......x..x..x..",
      "....X..g.g..X..g" "....X..g.g..X...",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.o.",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.", 0 },
    /* 15 NOLA FUNK: the kick and snare talk in 16ths */
    { 16, 2, 0,
      "x..x..x...x....." "x..x......x.x...",
      "....X..g.g.gX..." "....X.g..g..X.g.",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.",
      "b.x.b.x.b.x.b.x." "b.x.b.x.b.x.b.x.", 0 },
    /* 16 SWUNG BLUES-FUNK: a funk kick under a shuffle */
    { 12, 2, 0,
      "x....xx....." "x....xx...x.",
      "...X.g...X.g" "...X.g...X..",
      "x.xx.xx.xx.x" "x.xx.xx.xx.x",
      "x.xX.xx.xX.x" "x.xX.xx.xX.x", 0 },
    /* 17 SLOW NEO-FUNK: behind the beat, lots of air */
    { 16, 2, 0,
      "x......x..x....." "x.....x...x....x",
      "....X..g....X..." "....X.......X..g",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.",
      "x.x.x.x.x.x.x.x." "x.x.x.x.x.x.x.x.", 0 },
    /* 18 COUNTRY TWANG: a two-beat shuffle, the snare brushing between the backbeats */
    { 12, 1, 0,
      "x.....x.....",
      "..gX.g..gX.g",
      "x.xx.xx.xx.x",
      "x.xx.xx.xx.x", 0 },
    /* 19 SWAMP ROCK: 16ths on the hat, a heavy lazy backbeat */
    { 16, 2, 0,
      "x.....x.x......." "x.....x.x.....x.",
      "....X.......X..g" "....X.......X...",
      "XgxgXgxgXgxgXgxg" "XgxgXgxgXgxgXgxg",
      "X.x.X.x.X.x.X.x." "X.x.X.x.X.x.X.x.", 0 }
};

/* Fills. '~' in the snare lane: the beat plays on at that step; from the first other character the fill has the kit */
typedef struct {
    const char *snare, *aux, *kick;
} DrumFill;

#define NUM_FILLS_SMALL 3
#define NUM_FILLS_BIG   5
static const DrumFill FILLS_12[NUM_FILLS_SMALL + NUM_FILLS_BIG] = {
    { "~~~~~~~~~x.x", 0, 0 },
    { "~~~~~~~~~gxx", 0, 0 },
    { "~~~~~~~~~x..", "...........l", 0 },
    { "~~~~~~x.xxxx", 0, "......x....." },
    { "~~~~~~xxx...", ".........hhl", "......x....." },
    { "~~~~~~X.xx.x", "...........l", 0 },
    { "~~~~~~x.x...", ".........l.l", ".........x.." },
    { "~~~x.x~~~xxx", 0, 0 }
};
static const DrumFill FILLS_16[NUM_FILLS_SMALL + NUM_FILLS_BIG] = {
    { "~~~~~~~~~~~~x.xx", 0, 0 },
    { "~~~~~~~~~~~~xxxx", 0, 0 },
    { "~~~~~~~~~~~~x.x.", "..............l.", 0 },
    { "~~~~~~~~xxxxxxxx", 0, "........x......." },
    { "~~~~~~~~x.xx....", "............hhll", "........x......." },
    { "~~~~~~~~x..x..x.", 0, "..........x....." },
    { "~~~~~~~~xx.xx.xx", 0, 0 },
    { "~~~~~~~~x.x.....", "..........h.l.l.", "........x......." }
};

/* Riff bass lines */
typedef struct {
    uint8_t grid, nvar, nmove;
    const char *var[4][2];      /* two-bar shapes: [variant][bar A, bar B] while the chord stays */
    const char *move[4];        /* the bar before a chord change: ends on an approach note */
    const char *split;          /* a bar with two chords */
} BassRiff;

static const BassRiff BASS_RIFFS[NUM_BASS_STYLES - NUM_CLASSIC_BASS_STYLES] = {
    /* TX SHUFFLE: the box, walked in quarters with shuffled pickups */
    { 12, 4, 3,
      { { "1--3--5--6--", "7--6--5--3--" },
        { "1-13--5--6-6", "8--6--5-53--" },
        { "1-11-13--5-6", "7-76--5--3-3" },
        { "1--3-35--6--", "8-87--6--5-3" } },
      { "1--3--5--n--", "1--1--w--a--", "1--3--5-5n--", 0 },
      "1--n--1--n--" },
    /* CHI WALK: walking quarters */
    { 12, 4, 4,
      { { "1--3--5--6--", "8--6--5--3--" },
        { "1--2--3--5--", "6--5--3--2--" },
        { "1--3--4--d--", "5--6--7--5--" },
        { "1--V--S--Z--", "1--3--5-56--" } },
      { "1--3--5--n--", "1--5--w--a--", "1--7--6--n--", "1--2--3--n--" },
      "1--n--1--n--" },
    /* LUMP: long-short on the root (Jimmy Reed) */
    { 12, 2, 2,
      { { "1-11-11-11-1", "1-11-15-56-5" },
        { "1-11-1V-V1-1", "1-11-1V-VS-Z" }, { 0, 0 }, { 0, 0 } },
      { "1-11-11-1w-a", "1-11-15-5n--", 0, 0 },
      "1-1n--1-1n--" },
    /* SLOW BLUES: held roots, triplet pickups */
    { 12, 3, 2,
      { { "1--1-35--6-5", "8--7-65--3-2" },
        { "1-----5-----", "1-----5--S-Z" },
        { "1----13--5--", "6--5--3--1-V" }, { 0, 0 } },
      { "1--1-35--w-a", "1-----5-5n--", 0, 0 },
      "1--5-n1--5-n" },
    /* MEMPHIS: root, fifth, sixth, with room (Duck Dunn) */
    { 16, 3, 2,
      { { "1-....1-5---6-5-", "1-....1-5---V-S-" },
        { "1--1..5-6-5-..1-", "1--1..5-6-8-6-5-" },
        { "1---m-4-....m-4-", "1---m-4-..7-1-.." }, { 0, 0 } },
      { "1-....1-5---w-a-", "1--1..5-6-5-n-..", 0, 0 },
      "1---n---1---n---" },
    /* RHUMBA: 3 + 3 + 2 */
    { 16, 3, 2,
      { { "1-----3-----5---", "6-----5-----3---" },
        { "1-----5-----8---", "7-----5-----3---" },
        { "1-----1-..5-5-..", "1-----1-..5-6-5-" }, { 0, 0 } },
      { "1-----3-----n---", "1-----5-----n---", 0, 0 },
      "1-----n-1-----n-" },
    /* BOOGIE: the left hand of a boogie-woogie pianist */
    { 12, 4, 2,
      { { "1-35-67-65-3", "1-35-67-65-3" },
        { "1-35-68-65-3", "1-35-67-65-3" },
        { "1-35-65-35-6", "1-35-67-65-3" },
        { "1-13-35-56-6", "7-76-65-53-3" } },
      { "1-35-67-65-n", "1-35-65-3w-a", 0, 0 },
      "1-35-n1-35-n" },
    /* ROCK 8THS: a driving root with a turn at the end of the phrase */
    { 16, 3, 2,
      { { "1-1-1-1-1-1-1-1-", "1-1-1-1-1-1-5-7-" },
        { "1--1--1-1--1--1-", "1--1--1-1--5--7-" },
        { "1-1-1-1-1-1-1-1-", "1-1-1-1-1-5-6-7-" }, { 0, 0 } },
      { "1-1-1-1-1-1-w-a-", "1-1-1-1-1-1-1-n-", 0, 0 },
      "1-1-1-n-1-1-1-n-" },
    /* FUNK: the one, dead notes, the b7 and the octave */
    { 16, 3, 2,
      { { "1-.x..1-..7-1-..", "1-.x..1-..x-5-7-" },
        { "1-.8..x-1-..7-8-", "1-.8..x-1-..5-7-" },
        { "1-..x-1-.x7-..1-", "1-..x-1-.x5-6-7-" }, { 0, 0 } },
      { "1-.x..1-..7-w-a-", "1-.8..x-1-....n-", 0, 0 },
      "1-.x..n-1-.x..n-" },
    /* NOLA: second line, a tuba's line */
    { 16, 2, 2,
      { { "1--V--1-1--V--1-", "1--V--1-3--5--6-" },
        { "1---..5-1---..V-", "1---..5-6---5-3-" }, { 0, 0 }, { 0, 0 } },
      { "1--V--1-1--w--a-", "1---..5-1---n-..", 0, 0 },
      "1--5--n-1--5--n-" },
    /* COUNTRY: two-beat, root and fifth, a walk-up into the change */
    { 12, 3, 2,
      { { "1-----V-----", "1-----V-----" },
        { "1-----5-----", "1-----V--S-Z" },
        { "1-----V-----", "1--3--5--6--" }, { 0, 0 } },
      { "1-----V--w-a", "1-----5--n--", 0, 0 },
      "1--n--1--n--" },
    /* SWAMP: the root and the note under it */
    { 16, 2, 2,
      { { "1-----1-..Z-1---", "1-----1-..5-7---" },
        { "1--1--1-....Z-1-", "1--1--1-..Z-Z-1-" }, { 0, 0 }, { 0, 0 } },
      { "1-----1-..w-a---", "1--1--1-....n---", 0, 0 },
      "1-----n-1-----n-" },
    /* JAZZ WALK: scales and chromatics */
    { 12, 4, 4,
      { { "1--2--3--5--", "6--5--4--3--" },
        { "1--3--5--6--", "8--7--6--5--" },
        { "1--V--S--Z--", "1--2--m--3--" },
        { "1--3--5--3--", "4--d--5--7--" } },
      { "1--3--5--n--", "1--2--w--a--", "1--7--6--n--", "1--5-5w--a--" },
      "1--n--1--n--" }
};

/* ---- the grid ---- */

/* the tick (0..23 in a beat) of a grid's step */
static uint8_t grid_tick(uint8_t grid, uint8_t step, uint8_t swing, uint8_t fixed_12_8) {
    uint8_t off = fixed_12_8 ? 16u : (uint8_t)(12u + ((uint32_t)swing * 6u) / 100u);
    if (step == 0) return 0;
    if (step == 1) return (uint8_t)(off / 2u);
    if (step == 2) return off;
    (void)grid;
    return (uint8_t)(off + (24u - off) / 2u);
}

/* the grid's step at this tick, or -1 between steps */
static int8_t grid_step(uint8_t grid, uint8_t tick, uint8_t swing, uint8_t fixed_12_8) {
    uint8_t n = (grid == 12) ? 3u : 4u;
    for (uint8_t s = 0; s < n; s++) {
        if (grid_tick(grid, s, swing, fixed_12_8) == tick) return (int8_t)s;
    }
    return -1;
}

static uint32_t pat_hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 2654435761u + b * 40503u + 0x9E3779B9u;
    h ^= h >> 13; h *= 0x5BD1E995u; h ^= h >> 15;
    return h;
}

static uint8_t clamp_vel(int v) {
    return (uint8_t)((v < 1) ? 1 : (v > 127) ? 127 : v);
}

/* ---- drums ---- */

static const DrumFill *g_dr_fill = 0;
static uint8_t g_dr_fill_grid = 0;
static uint8_t g_dr_fill_dyn = 0;       /* a fill asked for mid-bar (heat step, outro): the last beat only */
static uint8_t g_dr_crash = 0;
static uint8_t g_dr_ride = 0;
static uint8_t g_dr_bar_ab = 0;
static uint8_t g_dr_bar = 0xFF;
static uint8_t g_dr_pat = 0xFF;

static void drum_hit(PerformanceFrame *frame, uint8_t voice, int vel) {
    frame->drum_mask |= (uint16_t)(1u << voice);
    if (clamp_vel(vel) > frame->drum_vel[voice]) frame->drum_vel[voice] = clamp_vel(vel);
}

static void drums_new_bar(const HarmonyState *hs, const DrumPat *pat, uint8_t pat_idx) {
    uint8_t d_var = hs->drums_variation, d_full = hs->drums_fullness;
    uint8_t total = harmony_form_bars(hs->form_mode);
    uint8_t last = (uint8_t)(hs->current_bar + 1u >= total);
    const DrumFill *set = (pat->grid == 12) ? FILLS_12 : FILLS_16;
    uint32_t h = pat_hash(hs->chorus_count, (uint32_t)hs->current_bar * 31u + pat_idx);

    g_dr_fill = 0;
    g_dr_fill_dyn = 0;
    g_dr_fill_grid = pat->grid;
    if (d_full >= 25 && !hs->is_vamp_mode) {
        if (last && d_var > 25) {
            /* the end of the chorus: a big fill, a small one while the variation is low */
            g_dr_fill = (d_var > 45) ? &set[NUM_FILLS_SMALL + h % NUM_FILLS_BIG] : &set[h % NUM_FILLS_SMALL];
        } else if (d_var > 65 && (hs->current_bar & 3u) == 3u) {
            g_dr_fill = &set[h % NUM_FILLS_SMALL];
        }
    }
    g_dr_bar_ab = (pat->bars == 2 && d_var > 20) ? (hs->current_bar & 1u) : 0u;
    if (pat->flags & DPF_RIDE_DEFAULT) g_dr_ride = 1;
    else if ((pat->flags & DPF_NO_RIDE) || !pat->ride) g_dr_ride = 0;
    else g_dr_ride = (d_var > 40 && d_full >= 45 && (hs->chorus_count & 1u) && hs->heat_level >= 3) ? 1u : 0u;
    g_dr_bar = hs->current_bar;
    g_dr_pat = pat_idx;
}

static void drums_generate(HarmonyState *hs, const PresetConfig *preset, PerformanceFrame *frame, int8_t j_vel) {
    uint8_t pat_idx = hs->drum_pattern % NUM_GROOVES;
    const DrumPat *pat = &DRUM_PATS[pat_idx];
    uint8_t fixed = (pat->flags & DPF_FIXED_12_8) ? 1u : 0u;
    int8_t step = grid_step(pat->grid, hs->current_subdiv, preset->swing, fixed);
    if (step < 0) return;
    uint8_t per = (uint8_t)(pat->grid / 4u);
    uint8_t pos = (uint8_t)(hs->current_beat * per + (uint8_t)step);
    uint8_t d_full = hs->drums_fullness;
    uint8_t d_var = hs->drums_variation;

    if (pos == 0 || g_dr_bar != hs->current_bar || g_dr_pat != pat_idx) {
        uint8_t crash = g_dr_crash;
        drums_new_bar(hs, pat, pat_idx);
        g_dr_crash = (pos == 0) ? crash : 0;
    }

    if (d_full < 20) {
        /* next to nothing: a cross-stick on 2 and 4 */
        if (step == 0 && (hs->current_beat == 1 || hs->current_beat == 3)) drum_hit(frame, DV_RIM, 82 + j_vel);
        g_dr_crash = 0;
        if (pos + 1u == pat->grid) hs->fill_pending = 0;
        return;
    }

    /* a fill asked for while the bar is already running (a heat step, the outro's pickup): its last beat */
    if (!g_dr_fill && pos == (uint8_t)(3u * per) && (hs->fill_pending || hs->ending_armed == ENDING_ARMED_NEXT_BAR)) {
        const DrumFill *set = (pat->grid == 12) ? FILLS_12 : FILLS_16;
        g_dr_fill = &set[pat_hash(hs->chorus_count, hs->current_bar) % NUM_FILLS_SMALL];
        g_dr_fill_grid = pat->grid;
        g_dr_fill_dyn = 1;
    }

    if (pos == 0 && g_dr_crash) {
        if (d_full >= 45) drum_hit(frame, DV_CRASH, 104 + j_vel);
        g_dr_crash = 0;
    }

    uint8_t in_fill = (g_dr_fill && g_dr_fill_grid == pat->grid && g_dr_fill->snare[pos] != '~');
    if (in_fill) {
        char s = g_dr_fill->snare[pos];
        char a = g_dr_fill->aux ? g_dr_fill->aux[pos] : '.';
        char k = g_dr_fill->kick ? g_dr_fill->kick[pos] : '.';
        int ramp = 82 + (30 * (int)pos) / (int)(pat->grid - 1u);
        if (s == 'X') drum_hit(frame, DV_SNARE, 118 + j_vel);
        else if (s == 'x') drum_hit(frame, DV_SNARE, ramp + j_vel);
        else if (s == 'g') drum_hit(frame, DV_SNARE, 52 + j_vel);
        if (a == 'h') drum_hit(frame, DV_TOM_HI, ramp + j_vel);
        else if (a == 'l') drum_hit(frame, DV_TOM_LO, ramp + 4 + j_vel);
        if (k == 'x' || (step == 0 && s == '.' && a == '.')) drum_hit(frame, DV_KICK, 100 + j_vel);
        if (pos + 1u == pat->grid) {
            g_dr_crash = 1;
            hs->fill_pending = 0;
            if (hs->ending_armed == ENDING_ARMED_NEXT_BAR) drum_hit(frame, DV_RIM, 85);
        }
        return;
    }
    if (pos + 1u == pat->grid) hs->fill_pending = 0;

    uint16_t idx = (uint16_t)(g_dr_bar_ab * pat->grid + pos);
    char k = pat->kick ? pat->kick[idx] : '.';
    char s = pat->snare ? pat->snare[idx] : '.';
    char t = '.';
    char a = pat->aux ? pat->aux[idx] : '.';
    uint8_t sparse = (d_full < 45);          /* the skeleton: kick, backbeat, the top line on the beats */
    uint8_t ghosts = (d_full >= 75);
    int lift = (d_full > 75) ? 6 : sparse ? -14 : 0;

    if (g_dr_ride) {
        t = pat->ride[idx];
        /* the hat with the foot on 2 and 4 under the ride */
        if (step == 0 && (hs->current_beat == 1 || hs->current_beat == 3)) drum_hit(frame, DV_HAT, 62 + j_vel);
        if (t == 'X' || t == 'b') drum_hit(frame, DV_RIDE, 108 + lift + j_vel);
        else if (t == 'x' && (!sparse || step == 0)) drum_hit(frame, DV_RIDE, 88 + lift + j_vel);
        else if (t == 'g' && ghosts) drum_hit(frame, DV_RIDE, 58 + j_vel);
    } else if (pat->hat) {
        t = pat->hat[idx];
        if (t == 'X') drum_hit(frame, DV_HAT, 108 + lift + j_vel);
        else if (t == 'x' && (!sparse || step == 0)) drum_hit(frame, DV_HAT, 88 + lift + j_vel);
        else if (t == 'g' && !sparse) drum_hit(frame, DV_HAT, 56 + j_vel);
        else if (t == 'p') drum_hit(frame, DV_HAT, 66 + j_vel);
        else if (t == 'o') {
            if (ghosts || d_var > 50) drum_hit(frame, DV_HAT_OPEN, 96 + j_vel);
            else if (!sparse) drum_hit(frame, DV_HAT, 88 + j_vel);
        }
    }

    if (k == 'X') drum_hit(frame, DV_KICK, 120 + lift + j_vel);
    else if (k == 'x' && (!sparse || step == 0)) drum_hit(frame, DV_KICK, 106 + lift + j_vel);
    else if (k == 'g' && !sparse) drum_hit(frame, DV_KICK, 66 + j_vel);

    if (s == 'X') drum_hit(frame, DV_SNARE, 112 + lift + j_vel);
    else if (s == 'x' && !sparse) drum_hit(frame, DV_SNARE, 94 + lift + j_vel);
    else if (s == 'g' && ghosts) drum_hit(frame, DV_SNARE, 40 + (d_var > 65 ? 8 : 0) + j_vel);
    else if (s == 'r') drum_hit(frame, DV_RIM, 88 + lift + j_vel);

    if (!sparse) {
        if (a == 'h') drum_hit(frame, DV_TOM_HI, 98 + j_vel);
        else if (a == 'l') drum_hit(frame, DV_TOM_LO, 102 + j_vel);
    }
}

/* ---- bass riffs ---- */

static const char *g_br_str = 0;
static uint8_t g_br_style = 0xFF;
static uint8_t g_br_bar = 0xFF;
static uint8_t g_br_run = 0;
static uint8_t g_br_var = 0;
static uint8_t g_br_last_deg = 0xFF;
static uint8_t g_br_last_note = 40;

static void bass_riff_new_bar(const HarmonyState *hs, const BassRiff *riff, uint8_t style, uint8_t fresh_bar) {
    BarSlot slot = harmony_get_current_bar_slot(hs);
    BarSlot next = harmony_get_next_bar_slot(hs);
    uint8_t b_var = hs->bass_variation;
    uint8_t last_deg = slot.has_split ? (uint8_t)slot.secondary : (uint8_t)slot.primary;
    uint32_t h = pat_hash(hs->chorus_count, (uint32_t)hs->current_bar * 17u + style);

    if (fresh_bar) {
        if ((uint8_t)slot.primary == g_br_last_deg && !slot.has_split) g_br_run++;
        else g_br_run = 0;
        g_br_last_deg = last_deg;
    }
    if (g_br_run == 0 || g_br_style != style || (b_var > 60 && (g_br_run & 1u) == 0)) {
        /* a new shape: the plain one while the variation is low, more of them as it rises. The chorus and the
         * bar pick it, so a chorus repeats the same way every time it comes round with the same settings */
        uint8_t n = (b_var <= 30) ? 1u : (b_var <= 60) ? 2u : riff->nvar;
        if (n > riff->nvar) n = riff->nvar;
        g_br_var = (uint8_t)(h % n);
    }
    if (slot.has_split) {
        g_br_str = riff->split;
    } else if ((uint8_t)next.primary != (uint8_t)slot.primary) {
        uint8_t n = (b_var <= 30) ? 1u : riff->nmove;
        g_br_str = riff->move[(h >> 8) % n];
    } else {
        g_br_str = riff->var[g_br_var][g_br_run & 1u];
    }
    g_br_style = style;
    g_br_bar = hs->current_bar;
}

static uint8_t bass_root_of(uint8_t pitch_class) {
    return (uint8_t)(40u + ((pitch_class % 12u) + 8u) % 12u);      /* E2..Eb3 */
}

/* a riff character as a note; 0: no note */
static uint8_t bass_riff_note(char c, uint8_t root, ChordQuality q, uint8_t target, uint8_t last) {
    uint8_t minor = (q == CHORD_MIN7 || q == CHORD_DIM7);
    int n, t = target;
    /* the next chord's root in the octave the line is in, so an approach note leads into it instead of leaping */
    if (c == 'a' || c == 'A' || c == 'w' || c == 'n') {
        while (t + 6 < (int)last && t + 12 <= 64) t += 12;
        while (t - 6 > (int)last && t - 12 >= 35) t -= 12;
    }
    switch (c) {
        case '1': n = root; break;
        case '2': n = root + 2; break;
        case 'm': n = root + 3; break;
        case '3': n = root + (minor ? 3 : 4); break;
        case '4': n = root + 5; break;
        case 'd': n = root + 6; break;
        case '5': n = root + ((q == CHORD_DIM7) ? 6 : 7); break;
        case '6': n = root + 9; break;
        case '7': n = root + ((q == CHORD_DIM7) ? 9 : 10); break;
        case '8': n = root + 12; break;
        case 'V': n = (root >= 45) ? root - 5 : root + 7; break;
        case 'S': n = (root >= 43) ? root - 3 : root + 9; break;
        case 'Z': n = (root >= 42) ? root - 2 : root + 10; break;
        case 'x': n = root; break;
        case 'a': n = ((int)last == t - 1) ? t + 1 : t - 1; break;
        case 'A': n = ((int)last == t + 1) ? t - 1 : t + 1; break;
        case 'w': n = t - 2; break;
        case 'n': n = ((int)last == t - 1) ? t + 1 : ((int)last == t + 1) ? t - 1 : ((int)last > t) ? t + 1 : t - 1; break;
        default: return 0;
    }
    while (n < 35) n += 12;
    while (n > 64) n -= 12;
    return (uint8_t)n;
}

static void bass_riff_generate(HarmonyState *hs, const PresetConfig *preset, PerformanceFrame *frame,
                               uint8_t style, uint8_t fixed_12_8, int8_t j_vel) {
    const BassRiff *riff = &BASS_RIFFS[style - NUM_CLASSIC_BASS_STYLES];
    int8_t step = grid_step(riff->grid, hs->current_subdiv, preset->swing, fixed_12_8);
    if (step < 0) return;
    uint8_t per = (uint8_t)(riff->grid / 4u);
    uint8_t pos = (uint8_t)(hs->current_beat * per + (uint8_t)step);
    uint8_t b_full = hs->bass_fullness;

    if (pos == 0 || g_br_bar != hs->current_bar || g_br_style != style || !g_br_str) {
        bass_riff_new_bar(hs, riff, style, pos == 0);
    }

    BarSlot slot = harmony_get_current_bar_slot(hs);
    BarSlot next = harmony_get_next_bar_slot(hs);
    uint8_t second_half = (hs->current_beat >= 2 && slot.has_split);
    ScaleDegreeInfo ci = harmony_get_degree_info(second_half ? slot.secondary : slot.primary);
    ScaleDegreeInfo ti = harmony_get_degree_info((slot.has_split && !second_half) ? slot.secondary : next.primary);
    uint8_t root = bass_root_of((uint8_t)(hs->root_semitone + ci.semitone_offset));
    uint8_t target = bass_root_of((uint8_t)(hs->root_semitone + ti.semitone_offset));

    char c;
    uint8_t gate;
    if (b_full < 20) {
        /* one note a bar */
        if (pos != 0) return;
        c = '1'; gate = 84;
    } else if (b_full < 40) {
        /* 1 and 3: the root, then the fifth (the way into the next chord when it changes) */
        if (pos == 0) c = '1';
        else if (pos == (uint8_t)(2u * per)) c = (slot.has_split) ? '1' : ((uint8_t)next.primary != (uint8_t)slot.primary) ? 'n' : '5';
        else return;
        gate = 42;
    } else {
        c = g_br_str[pos];
        if (c == '-' || c == '.') return;
        /* the note's length: itself and the holds after it; a rest cuts it short, another note ties into it */
        uint8_t end = (uint8_t)(pos + 1u);
        while (end < riff->grid && g_br_str[end] == '-') end++;
        uint16_t t0 = (uint16_t)((pos / per) * 24u + grid_tick(riff->grid, (uint8_t)(pos % per), preset->swing, fixed_12_8));
        uint16_t t1 = (end >= riff->grid) ? 96u
                    : (uint16_t)((end / per) * 24u + grid_tick(riff->grid, (uint8_t)(end % per), preset->swing, fixed_12_8));
        uint16_t len = (uint16_t)(t1 - t0);
        uint8_t legato = (end < riff->grid && g_br_str[end] != '.');
        if (!legato && len > 3u) len = (uint16_t)(len - len / 6u);
        gate = (uint8_t)((len > 200u) ? 200u : (len < 2u) ? 2u : len);
    }

    uint8_t note = bass_riff_note(c, root, ci.quality, target, g_br_last_note);
    if (!note) return;
    int vel = (pos == 0) ? 118 : (step == 0) ? 106 : 94;
    if (c == 'x') { vel = 62; gate = 2; frame->bass_ghost = 1; }
    vel += j_vel + ((b_full > 75) ? 6 : (b_full < 40) ? -8 : 0);
    frame->bass_trigger = 1;
    frame->bass_note = note;
    frame->bass_vel = clamp_vel(vel);
    frame->bass_gate_ticks = gate;
    if (c != 'x') g_br_last_note = note;
}
