#ifndef DSP_VOICES_H
#define DSP_VOICES_H

#include <stdint.h>
#include "blues_types.h"
#include "blues_arranger.h"

void dsp_init(void);
void dsp_render_block(int32_t *buf, uint32_t num_frames, const PresetConfig *preset);
void dsp_note_on_bass(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode);
void dsp_note_on_keys(const Voicing *voicing, uint8_t velocity, uint8_t tuning_mode);
void dsp_extend_drone_gate(uint32_t gate_samples);
void dsp_note_on_lead(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode);
void dsp_note_on_lead_ex(uint8_t key_note, uint8_t target_note, uint8_t velocity, uint8_t tuning_mode);
void dsp_note_off_lead(uint8_t midi_note);
void dsp_set_lead_play_style(uint8_t play_style);
void dsp_trigger_drum(uint16_t mask, const uint8_t *drum_vel);     /* mask: bit DV_*, drum_vel[NUM_DRUM_VOICES] */
void dsp_note_on_bass_ex(uint8_t midi_note, uint8_t velocity, uint8_t tuning_mode, uint32_t gate_samples, uint8_t ghost);
void dsp_all_notes_off(void);

#endif // DSP_VOICES_H
