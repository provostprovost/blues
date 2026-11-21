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
    FORM_I_bVII_IV
} FormMode;

#define NUM_FORM_MODES 6

typedef enum {
    VAR_FLAT_FOUR = 0,
    VAR_QUICK_CHANGE,
    VAR_145_BOUNCE,
    VAR_DIM_WALKUP,
    VAR_PICKUP_INTRO
} OpeningVar;

typedef enum {
    OUTRO_CLASSIC_SLIDE = 0, // 2-bar walkdown + #I9 -> I9 half-step slide
    OUTRO_V_bV_IV_I,         // 2-bar V9 -> bV9 -> IV9 -> I9 breakdown
    OUTRO_IV_I_TURN,         // 2-bar IV7 -> #IVdim7 -> I7 -> VI7 -> ii7 -> V7 -> I9
    OUTRO_QUICK_1BAR,        // 1-bar V7 -> bV7 -> IV7 -> #I9 -> I9 quick tag
    OUTRO_STINGER_ONLY       // 0-bar Beat-1 I9 stinger + immediate stop
} OutroType;

#define NUM_OUTRO_TYPES 5

typedef enum {
    INTRO_OFF = 0,
    INTRO_AUTO,              // Style-matched intro for current groove
    INTRO_V7_1BAR,           // 1-bar V7 turnaround lick
    INTRO_TURNAROUND_2BAR,   // 2-bar classic blues contrapuntal turnaround
    INTRO_FUNK_2BAR,         // 2-bar syncopated funk/soul pickup
    INTRO_BAND_4BAR,         // 4-bar V7 | IV7 | I7-IV7 | I7-V7 intro
    INTRO_CLICK_1BAR,        // 1-bar rimshot count-in
    INTRO_DRONE_1BAR         // 1-bar downbeat root+5th drone + 2, 3, 4 ticks
} IntroMode;

#define NUM_INTRO_MODES 8

typedef enum {
    ENDING_NONE = 0,
    ENDING_ARMED_NEXT_BAR,   // Armed: enters outro on the very next bar boundary
    ENDING_BAR_1,            // Playing Bar 1 of 2-bar outro riff
    ENDING_BAR_2,            // Playing Bar 2 of 2-bar outro (or 1-bar quick outro)
    ENDING_STINGER           // Beat-1 final I9 stinger + stop
} EndingState;

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
    GROOVE_BOOGIE_WOOGIE,
    GROOVE_PURDIE_SHUFFLE,
    GROOVE_TRAIN_BEAT,
    GROOVE_BO_DIDDLEY,
    GROOVE_JAZZ_SHUFFLE,
    GROOVE_GOSPEL_SWING,
    GROOVE_DELTA_STOMP,
    GROOVE_POCKET_FUNK,
    GROOVE_NOLA_FUNK,
    GROOVE_BLUES_FUNK,
    GROOVE_SLOW_NEO_FUNK,
    GROOVE_COUNTRY_TWANG,
    GROOVE_SWAMP_ROCK
} GrooveStyle;

#define NUM_DRUM_PATTERNS 20

typedef enum {
    BASS_P_BASS = 0,
    BASS_UPRIGHT,
    BASS_MUTED_TICTAC,
    BASS_MOOG_FUNK,
    BASS_JAZZ_60S,
    BASS_FLATWOUND,
    BASS_AMPEG_GROWL,
    BASS_SUB_OCTAVE,
    BASS_LATELY_FM,
    BASS_SLAP_POP,
    BASS_HOFNER,
    BASS_ACID_303,
    BASS_TUBA,
    BASS_PICK_ROCK
} BassInstrument;

#define NUM_BASS_INSTRUMENTS 14

typedef enum {
    BASS_STYLE_WALKING = 0,
    BASS_STYLE_SHUFFLE_REP,
    BASS_STYLE_BOOGIE_BOX,
    BASS_STYLE_ROOT_FIFTH,
    BASS_STYLE_SYNCOPATED,
    BASS_STYLE_RUMBA,
    BASS_STYLE_GOSPEL_WALK,
    BASS_STYLE_SLOW_12_8,
    BASS_STYLE_MOTOWN,
    BASS_STYLE_SECOND_LINE,
    BASS_STYLE_FUNK_16,
    BASS_STYLE_COUNTRY_2
} BassStyle;

#define NUM_BASS_STYLES 12

typedef enum {
    KEYS_STYLE_CHARLESTON = 0,
    KEYS_STYLE_BACKBEAT,
    KEYS_STYLE_CALL_RESP,
    KEYS_STYLE_PAD,
    KEYS_STYLE_CHOP_4,
    KEYS_STYLE_STRIDE,
    KEYS_STYLE_BOOGIE_8,
    KEYS_STYLE_GOSPEL_SWELL,
    KEYS_STYLE_RUMBA_MONTUNO,
    KEYS_STYLE_SLOW_12_8,
    KEYS_STYLE_FUNK_CLAV,
    KEYS_STYLE_COUNTRY_CHOP
} KeysStyle;

#define NUM_KEYS_STYLES 12

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
    KEYS_LESLIE_ORGAN,
    KEYS_ACCORDION,
    KEYS_VIBES,
    KEYS_FARFISA,
    KEYS_MELLOTRON,
    KEYS_CP70,
    KEYS_HARMONIUM
} ChordInstrument;

#define NUM_KEYS_INSTRUMENTS 18

typedef enum {
    DRUM_STUDIO = 0,
    DRUM_BRUSH,
    DRUM_RETRO_808,
    DRUM_STAX_TIGHT,
    DRUM_CHICAGO_ROOM,
    DRUM_DRY_VINTAGE,
    DRUM_JAZZ_CLUB,
    DRUM_HEAVY_ROCK,
    DRUM_CR78,
    DRUM_NEW_ORLEANS,
    DRUM_LINN_GATE,
    DRUM_LOFI_BOX
} DrumKit;

#define NUM_DRUM_KITS 12

typedef enum {
    LEAD_BLUES_GUITAR = 0,
    LEAD_HARMONICA,
    LEAD_ALTO_SAX,
    LEAD_TENOR_SAX,
    LEAD_BARI_SAX,
    LEAD_MUTED_TRUMPET,
    LEAD_HORN_SECT,
    LEAD_JAZZ_FLUTE,
    LEAD_WHISTLE,
    LEAD_MOOG_LEAD,
    LEAD_WARM_JAZZ,
    LEAD_FUZZ_OCTAVE,
    LEAD_PERC_ORGAN,
    LEAD_ROCK_B3,
    LEAD_TALKBOX,
    LEAD_SLIDE_GUITAR,
    LEAD_WAH_GUITAR,
    LEAD_TELE_TWANG,
    LEAD_FIDDLE,
    LEAD_BANJO,
    LEAD_WAVEFOLDER,
    LEAD_HARD_SYNC,
    LEAD_SID_CHIP,
    LEAD_STEELPAN,
    LEAD_THEREMIN
} LeadInstrument;

#define NUM_LEAD_INSTRUMENTS 25

typedef enum {
    LEAD_PLAY_POLY = 0,
    LEAD_PLAY_LEGATO_GLIDE,
    LEAD_PLAY_BLUES_BEND,
    LEAD_PLAY_OCTAVE_STACK
} LeadPlayStyle;

#define NUM_LEAD_PLAY_STYLES 4

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
    uint8_t     variation;
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
    uint8_t     intro_total_bars;
    IntroMode   active_intro_style;
    uint8_t     hold_chord_active;
    uint8_t     is_playing;
    uint8_t     ui_dirty;
    uint8_t     ext_clock_active;
    volatile uint8_t ext_clock_ticks_pending;
    uint16_t    ext_clock_timeout_ms;
    uint8_t     cpu_load_pct;
    uint16_t    chorus_count;
    uint8_t     fullness;
    uint8_t     bass_fullness;
    uint8_t     keys_fullness;
    uint8_t     drums_fullness;
    uint8_t     bass_variation;
    uint8_t     keys_variation;
    uint8_t     drums_variation;
    uint8_t     bass_style;
    uint8_t     keys_style;
    uint8_t     drum_pattern;
    uint8_t     easy_lead;
    uint8_t     heat_level;
    volatile uint8_t vis_bass_pulse;
    volatile uint8_t vis_keys_pulse;
    volatile uint8_t vis_drum_pulse;
    char        toast_msg[28];
    uint32_t    toast_until_ms;
} HarmonyState;

typedef struct {
    uint8_t  magic;
    uint8_t  preset_id;
    char     name[20];
    uint8_t  root_key;
    uint8_t  groove_style;
    uint8_t  bpm;
    uint8_t  variation;
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
    uint8_t  fullness;
    uint8_t  bass_fullness;
    uint8_t  keys_fullness;
    uint8_t  drums_fullness;
    uint8_t  bass_variation;
    uint8_t  keys_variation;
    uint8_t  drums_variation;
    uint8_t  bass_style;
    uint8_t  keys_style;
    uint8_t  drum_pattern;
    uint8_t  lead_play_style;
    uint8_t  easy_lead;
    uint8_t  heat_level;
} PresetConfig;

typedef struct {
    uint32_t magic;
    uint8_t  active_preset;
    uint8_t  master_tuning;
    uint8_t  count_in_enabled;
    uint8_t  easy_lead;
} GlobalSettings;

#endif // BLUES_TYPES_H
