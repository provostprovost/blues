#ifndef BLUES_TYPES_H
#define BLUES_TYPES_H

#include <stdint.h>
#include "app_config.h"

typedef enum {
    DEG_I7 = 0,
    DEG_IV7,
    DEG_V7,
    DEG_i7,
    DEG_iv7,
    DEG_v7,
    DEG_ii7,
    DEG_vi7,
    DEG_VI7,
    DEG_bVI7,
    DEG_bVII7,
    DEG_SHARP_IV_DIM7
} ScaleDegree;

typedef enum {
    CHORD_DOM7 = 0,
    CHORD_MIN7,
    CHORD_DIM7
} ChordQuality;

typedef struct {
    uint8_t      root_note;
    ChordQuality quality;
} ChordContext;

typedef enum {
    FORM_12BAR = 0,
    FORM_QUICK_CHANGE,
    FORM_145_OPENING,
    FORM_8BAR,
    FORM_MINOR_BLUES,
    FORM_I_IV_VAMP
} FormMode;

typedef enum {
    VAR_FLAT_FOUR = 0,
    VAR_QUICK_CHANGE,
    VAR_145_BOUNCE,
    VAR_DIM_WALKUP,
    VAR_PICKUP_INTRO
} OpeningVar;

typedef enum {
    OUTRO_TAG = 0,
    OUTRO_TRIPLE,
    OUTRO_CHROMATIC,
    OUTRO_STOP
} OutroType;

typedef enum {
    CUE_NONE = 0,
    CUE_TO_12BAR,
    CUE_TO_VAMP,
    CUE_HOLD_CHORD,
    CUE_FORCE_TURNAROUND,
    CUE_STOP_TIME,
    CUE_DRUM_FILL,
    CUE_ENDING_TAG
} CueAction;

typedef enum {
    GROOVE_SLOW_12_8 = 0,
    GROOVE_TEXAS_SHUFFLE,
    GROOVE_CHICAGO_SWING,
    GROOVE_STRAIGHT_ROCK,
    GROOVE_SECOND_LINE,
    GROOVE_MEMPHIS_SOUL,
    GROOVE_RHUMBA_BLUES,
    GROOVE_BOOGIE_WOOGIE
} GrooveStyle;

typedef enum {
    BASS_P_BASS = 0,
    BASS_UPRIGHT,
    BASS_MUTED_TICTAC,
    BASS_MOOG_FUNK
} BassInstrument;

typedef enum {
    KEYS_RHODES = 0,
    KEYS_BRIGHT_EP,
    KEYS_WURLI,
    KEYS_B3_CLEAN,
    KEYS_B3_DIRTY,
    KEYS_CLAVINET,
    KEYS_ARCHTOP_GUITAR,
    KEYS_BLUES_HORNS,
    KEYS_HONKY_TONK,
    KEYS_TREMOLO_EP,
    KEYS_RESONATOR,
    KEYS_LESLIE_ORGAN
} ChordInstrument;

typedef enum {
    DRUM_STUDIO = 0,
    DRUM_BRUSH,
    DRUM_RETRO_808,
    DRUM_STAX_TIGHT
} DrumKit;

typedef enum {
    LEAD_BLUES_GUITAR = 0,
    LEAD_HARMONICA,
    LEAD_TENOR_SAX,
    LEAD_MOOG_LEAD,
    LEAD_WARM_JAZZ,
    LEAD_FUZZ_OCTAVE,
    LEAD_MUTED_TRUMPET,
    LEAD_PERC_ORGAN
} LeadInstrument;

typedef struct {
    ScaleDegree  degree;
    ChordQuality quality;
    uint8_t      semitone_offset;
} ScaleDegreeInfo;

typedef struct {
    ScaleDegree primary;
    ScaleDegree secondary;
    uint8_t     has_split;
} BarSlot;

typedef struct {
    uint8_t notes[4];
    uint8_t count;
} Voicing;

typedef enum {
    ZONE_ROOT_SELECT = 0,
    ZONE_FLOW_CONTROL,
    ZONE_FORM_VARIATION,
    ZONE_PERF_TRIGGERS
} KeyZone;

typedef struct {
    uint8_t     root_semitone;
    FormMode    form_mode;
    OpeningVar  opening_var;
    OutroType   outro_type;
    uint8_t     current_bar;
    uint8_t     current_beat;
    uint8_t     current_subdiv;
    CueAction   queued_cue;
    uint8_t     spice_level;
    uint8_t     is_vamp_mode;
    uint8_t     vamp_bar_counter;
    uint8_t     stop_time_counter;
    uint8_t     ending_armed;
    uint8_t     queued_next_key;
    uint8_t     has_queued_key;
    uint8_t     mod_next_bar;
    uint8_t     manual_stab_pending;
    uint8_t     fill_pending;
    uint8_t     count_in_enabled;
    uint8_t     count_in_bars_remaining;
    uint8_t     hold_chord_active;
    uint8_t     is_playing;
    uint8_t     ui_dirty;
    uint8_t     ext_clock_active;
    volatile uint8_t ext_clock_ticks_pending;
    uint16_t    ext_clock_timeout_ms;
    uint8_t     cpu_load_pct;
    uint16_t    chorus_count;
} HarmonyState;

typedef struct {
    uint8_t  magic;
    uint8_t  preset_id;
    char     name[16];
    uint8_t  root_key;
    uint8_t  groove_style;
    uint8_t  bpm;
    uint8_t  spice;
    uint8_t  swing;
    uint8_t  ep_tone_algo;
    uint8_t  bass_vol;
    uint8_t  keys_vol;
    uint8_t  drums_vol;
    uint8_t  reverb_mix;
    uint8_t  master_tuning;
    uint8_t  bass_cutoff;
    uint8_t  drive_level;
    uint8_t  count_in_enabled;
    uint8_t  form_mode;
    uint8_t  opening_var;
    uint8_t  outro_type;
    uint8_t  bass_instrument;
    uint8_t  drum_kit;
    uint8_t  lead_instrument;
    uint8_t  lead_vol;
} PresetConfig;

typedef struct {
    uint32_t magic;
    uint8_t  active_preset;
    uint8_t  master_tuning;
    uint8_t  count_in_enabled;
    uint8_t  reserved;
} GlobalSettings;

#endif // BLUES_TYPES_H
