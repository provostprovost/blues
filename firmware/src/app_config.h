#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define TB1_VERSION_MAJOR         1
#define TB1_VERSION_MINOR         0
#undef APP_NAME
#define APP_NAME                  "TB-1"
#define FELUCCA_ID                "FM-1_960"

#define AUDIO_SAMPLE_RATE         44100
#define AUDIO_BLOCK_SIZE          256
#define MAX_EP_VOICES             4
#define MAX_DRUM_CHANNELS         4
#define NUM_PRESET_SLOTS          16
#define PRESET_BLOB_SLOT          0
#define SETTINGS_MAGIC            0x54423130u /* 'TB10' */
#define PRESET_MAGIC              0x5Cu
#define AUTOSAVE_DEBOUNCE_MS      2000
#define DOUBLE_TAP_WINDOW_MS      250

#endif // APP_CONFIG_H
