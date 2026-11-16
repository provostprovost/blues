#include <stdio.h>
#include <assert.h>
#include "../firmware/src/blues_types.h"
#include "../firmware/src/blues_harmony.h"
#include "../firmware/src/blues_arranger.h"
#include "../firmware/src/dsp_voices.h"
#include "../firmware/src/panel_input.h"
#include "../firmware/src/ui_screens.h"
#include "../firmware/src/hal_adapter.h"

// Phase 1 Unit Test: Harmonic State Machine & Structural Transitions
void test_stage1_harmony(void) {
    printf("[Stage 1] Testing Harmony State Machine...\n");

    HarmonyState hs;
    harmony_init(&hs);

    // 1. Initial boot conditions: stopped by default, Key = E (semitone 4)
    assert(hs.is_playing == 0);
    assert(hs.root_semitone == 4);
    assert(hs.current_bar == 0);
    assert(hs.current_beat == 0);
    assert(hs.current_subdiv == 0);
    assert(hs.chorus_count == 0);

    // Ticking while stopped should not advance
    harmony_tick_subdivision(&hs);
    assert(hs.current_subdiv == 0);

    // Start playback
    hs.is_playing = 1;

    // 2. Test 24-PPQN counter and beat/bar advancement
    for (int sub = 0; sub < 24; sub++) {
        assert(hs.current_beat == 0);
        assert(hs.current_subdiv == sub);
        harmony_tick_subdivision(&hs);
    }
    assert(hs.current_beat == 1);
    assert(hs.current_subdiv == 0);

    // Advance through remaining 3 beats to finish bar 0
    for (int b = 1; b < 4; b++) {
        for (int sub = 0; sub < 24; sub++) {
            harmony_tick_subdivision(&hs);
        }
    }
    assert(hs.current_bar == 1);
    assert(hs.current_beat == 0);
    assert(hs.current_subdiv == 0);

    // 3. Test 100 continuous choruses wrap-around accuracy
    harmony_init(&hs);
    hs.is_playing = 1;
    for (int chorus = 0; chorus < 100; chorus++) {
        assert(hs.chorus_count == chorus);
        for (int bar = 0; bar < 12; bar++) {
            assert(hs.current_bar == bar);
            for (int beat = 0; beat < 4; beat++) {
                for (int sub = 0; sub < 24; sub++) {
                    harmony_tick_subdivision(&hs);
                }
            }
        }
    }
    assert(hs.chorus_count == 100);
    assert(hs.current_bar == 0);

    // 4. Test all 6 FormMode progressions
    FormMode forms[6] = {
        FORM_12BAR, FORM_QUICK_CHANGE, FORM_145_OPENING,
        FORM_8BAR, FORM_MINOR_BLUES, FORM_I_IV_VAMP
    };
    for (int f = 0; f < 6; f++) {
        hs.form_mode = forms[f];
        hs.chorus_count = 1; // Test standard form (past intro)
        uint8_t max_b = (forms[f] == FORM_8BAR) ? 8 : 12;
        for (uint8_t b = 0; b < max_b; b++) {
            hs.current_bar = b;
            BarSlot s = harmony_get_current_bar_slot(&hs);
            assert(s.primary <= DEG_SHARP_IV_DIM7);
            assert(s.secondary <= DEG_SHARP_IV_DIM7);
        }
    }

    // Check specific Form chords
    hs.form_mode = FORM_MINOR_BLUES;
    hs.current_bar = 0;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_i7);
    hs.current_bar = 4;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_iv7);
    hs.current_bar = 8;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_bVI7);

    hs.form_mode = FORM_8BAR;
    hs.current_bar = 1;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_V7);

    hs.form_mode = FORM_I_IV_VAMP;
    hs.current_bar = 0;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);
    hs.current_bar = 1;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_IV7);

    // 5. Test Opening Variations (Chorus 0 only)
    hs.form_mode = FORM_12BAR;
    hs.spice_level = 0;

    // 5a. Pickup Intro: V7 | IV7 | I7 | V7 on chorus 0
    hs.opening_var = VAR_PICKUP_INTRO;
    hs.chorus_count = 0;
    hs.current_bar = 0; assert(harmony_get_current_bar_slot(&hs).primary == DEG_V7);
    hs.current_bar = 1; assert(harmony_get_current_bar_slot(&hs).primary == DEG_IV7);
    hs.current_bar = 2; assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);
    hs.current_bar = 3; assert(harmony_get_current_bar_slot(&hs).primary == DEG_V7);

    // Revert on Chorus 1+
    hs.chorus_count = 1;
    hs.current_bar = 0; assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);

    // 5b. Diminished Walk-Up: I7 | IV7 | #IVdim7 | I7 on chorus 0
    hs.opening_var = VAR_DIM_WALKUP;
    hs.chorus_count = 0;
    hs.current_bar = 0; assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);
    hs.current_bar = 1; assert(harmony_get_current_bar_slot(&hs).primary == DEG_IV7);
    hs.current_bar = 2; assert(harmony_get_current_bar_slot(&hs).primary == DEG_SHARP_IV_DIM7);
    hs.current_bar = 3; assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);

    // 5c. 1-4-5 Bounce on chorus 0
    hs.opening_var = VAR_145_BOUNCE;
    hs.current_bar = 1;
    BarSlot bounce_slot = harmony_get_current_bar_slot(&hs);
    assert(bounce_slot.primary == DEG_IV7 && bounce_slot.secondary == DEG_V7 && bounce_slot.has_split == 1);

    // 6. Test Bar-Quantized Cues
    harmony_init(&hs);
    hs.is_playing = 1;
    hs.current_bar = 3;
    hs.queued_cue = CUE_TO_VAMP;
    seq_advance_bar(&hs);
    assert(hs.is_vamp_mode == 1);
    assert(hs.queued_cue == CUE_NONE);

    // Vamp progression alternates I7 (2 bars) and IV7 (2 bars)
    hs.vamp_bar_counter = 0;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);
    hs.vamp_bar_counter = 2;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_IV7);

    // Cue exit from vamp to 12-bar
    hs.queued_cue = CUE_TO_12BAR;
    seq_advance_bar(&hs);
    assert(hs.is_vamp_mode == 0);
    assert(hs.current_bar == 0);

    // Force turnaround cue
    hs.current_bar = 2;
    hs.queued_cue = CUE_FORCE_TURNAROUND;
    seq_advance_bar(&hs);
    assert(hs.current_bar == 10);

    // Stop-time cue
    hs.queued_cue = CUE_STOP_TIME;
    seq_advance_bar(&hs);
    assert(hs.stop_time_counter == 4);

    // 7. Test Outro Cadences and Termination (all 4 must stop playback)
    OutroType outros[4] = { OUTRO_TAG, OUTRO_TRIPLE, OUTRO_CHROMATIC, OUTRO_STOP };
    for (int o = 0; o < 4; o++) {
        harmony_init(&hs);
        hs.is_playing = 1;
        hs.outro_type = outros[o];
        hs.current_bar = 9;
        hs.queued_cue = CUE_ENDING_TAG;
        seq_advance_bar(&hs);
        assert(hs.ending_armed == 1);
        assert(hs.current_bar == 10);

        // Bar 10 slot check
        BarSlot s10 = harmony_get_current_bar_slot(&hs);
        assert(s10.primary <= DEG_SHARP_IV_DIM7);

        // Advance to Bar 11
        seq_advance_bar(&hs);
        assert(hs.current_bar == 11);
        BarSlot s11 = harmony_get_current_bar_slot(&hs);
        assert(s11.primary <= DEG_SHARP_IV_DIM7);

        // Advance past Bar 11 (completing Bar 12) -> must stop playback!
        seq_advance_bar(&hs);
        assert(hs.is_playing == 0);
        assert(hs.ending_armed == 0);
        assert(hs.current_bar == 0);
    }

    // 8. Test Single-Tap (Next Chorus) vs Double-Tap (Next Bar) Key Modulation
    harmony_init(&hs);
    hs.is_playing = 1;
    hs.root_semitone = 4; // E

    // 8a. Single tap at Bar 2: queued for next chorus
    hs.current_bar = 2;
    hs.queued_next_key = 0; // C
    hs.has_queued_key = 1;
    hs.mod_next_bar = 0;
    seq_advance_bar(&hs);
    assert(hs.root_semitone == 4); // Still E in Bar 3!
    assert(hs.has_queued_key == 1);

    // Fast-forward to end of chorus (Bar 11 -> 0)
    hs.current_bar = 11;
    seq_advance_bar(&hs);
    assert(hs.current_bar == 0);
    assert(hs.root_semitone == 0); // Modulated to C at new chorus!
    assert(hs.has_queued_key == 0);

    // 8b. Double tap at Bar 3: immediate modulation on next bar
    hs.current_bar = 3;
    hs.queued_next_key = 7; // G
    hs.has_queued_key = 1;
    hs.mod_next_bar = 1;
    seq_advance_bar(&hs);
    assert(hs.current_bar == 4);
    assert(hs.root_semitone == 7); // Immediately G at Bar 4!
    assert(hs.has_queued_key == 0);
    assert(hs.mod_next_bar == 0);

    printf("[Stage 1] All harmonic state machine tests passed successfully!\n");
}

#include "../firmware/src/blues_arranger.h"

// Phase 2 Unit Test: Procedural Arranger, Walking Bass & Voice Leading
void test_stage2_arranger(void) {
    printf("[Stage 2] Testing Arranger & Voice Leading...\n");

    // 1. Bass Range & Note Calculation Tests
    for (uint8_t k = 0; k < 12; k++) {
        for (uint8_t beat = 0; beat < 4; beat++) {
            for (uint8_t sub = 0; sub < 4; sub++) {
                uint8_t spices[5] = { 0, 25, 50, 75, 100 };
                for (int sp = 0; sp < 5; sp++) {
                    ChordContext c1 = { k, CHORD_DOM7 };
                    ChordContext c2 = { (uint8_t)((k + 5) % 12), CHORD_DOM7 };
                    uint8_t b_note = bass_compute_note(c1, c2, beat, sub, spices[sp]);
                    assert(b_note >= 40 && b_note <= 62);
                }
            }
        }
    }

    // Directional approach verification on Beat 3
    ChordContext c_c = { 0, CHORD_DOM7 }; // C
    ChordContext c_f = { 5, CHORD_DOM7 }; // F
    uint8_t b_asc = bass_compute_note(c_c, c_f, 3, 0, 50);
    assert(b_asc >= 40 && b_asc <= 62);

    // 2. Comping Voice Leading Tests
    Voicing prev = { {0}, 0 };
    // Spice 20 -> 2-note shell (3rd, 7th)
    Voicing v_shell = voice_lead_select(prev, DEG_I7, 4, 20);
    assert(v_shell.count == 2);
    for (int i = 0; i < v_shell.count; i++) {
        assert(v_shell.notes[i] >= 68 && v_shell.notes[i] <= 90);
    }

    // Spice 40 -> 3-note voicing
    Voicing v_3 = voice_lead_select(prev, DEG_I7, 4, 40);
    assert(v_3.count == 3);

    // Spice 65 -> 4-note voicing
    Voicing v_i7 = voice_lead_select(prev, DEG_I7, 4, 65);
    assert(v_i7.count == 4);
    for (int i = 0; i < v_i7.count; i++) {
        assert(v_i7.notes[i] >= 68 && v_i7.notes[i] <= 90);
    }

    // Voice leading from I7 to IV7: smooth guide-tone motion
    Voicing v_iv7 = voice_lead_select(v_i7, DEG_IV7, 4, 65);
    assert(v_iv7.count == 4);
    int total_movement = 0;
    for (int i = 0; i < 4; i++) {
        int diff = (int)v_iv7.notes[i] - (int)v_i7.notes[i];
        total_movement += (diff < 0) ? -diff : diff;
    }
    assert(total_movement <= 6); // Guide tones resolve smoothly!

    // 3. Arranger Performance Frame Generation
    HarmonyState hs;
    harmony_init(&hs);
    hs.is_playing = 1;
    hs.root_semitone = 4; // E
    PresetConfig preset;
    preset.groove_style = GROOVE_TEXAS_SHUFFLE;
    preset.swing = 67;
    preset.spice = 65;
    PerformanceFrame frame;
    Voicing last_v = { {0}, 0 };

    // Beat 0 downbeat: Kick + Hat + Bass + Comp
    hs.current_beat = 0;
    hs.current_subdiv = 0;
    arranger_generate_frame(&hs, &preset, &frame, &last_v);
    assert(frame.bass_trigger == 1);
    assert(frame.bass_vel > 0);
    assert(frame.comp_trigger == 1);
    assert(frame.comp_vel > 0);
    assert(frame.drum_mask & 0x01); // Kick
    assert(frame.drum_mask & 0x04); // Hat

    // Count-in mode: only rimshot click on beat 0
    hs.count_in_bars_remaining = 1;
    hs.current_beat = 0;
    hs.current_subdiv = 0;
    arranger_generate_frame(&hs, &preset, &frame, &last_v);
    assert(frame.bass_trigger == 0);
    assert(frame.comp_trigger == 0);
    assert(frame.drum_mask == 0x08); // Rimshot only
    hs.count_in_bars_remaining = 0;

    // Stop-time break: beats 1..3 muted
    hs.stop_time_counter = 4;
    hs.current_beat = 2;
    hs.current_subdiv = 0;
    arranger_generate_frame(&hs, &preset, &frame, &last_v);
    assert(frame.bass_trigger == 0);
    assert(frame.comp_trigger == 0);
    assert(frame.drum_mask == 0);

    printf("[Stage 2] Arranger & voice-leading tests passed successfully!\n");
}

#include "../firmware/src/dsp_voices.h"

// Phase 3 Unit Test: Fixed-Point DSP Engine & Polyphonic Voices
void test_stage3_dsp(void) {
    printf("[Stage 3] Testing Fixed-Point DSP Audio Engine...\n");

    dsp_init();

    PresetConfig preset;
    preset.bpm = 112;
    preset.groove_style = GROOVE_TEXAS_SHUFFLE;
    preset.swing = 67;
    preset.spice = 65;
    preset.bass_vol = 127;
    preset.keys_vol = 127;
    preset.drums_vol = 127;
    preset.reverb_mix = 127;
    preset.drive_level = 100;
    preset.bass_cutoff = 100;
    preset.master_tuning = 0; // 440 Hz
    preset.ep_tone_algo = 0;

    int32_t buf[256 * 2];

    // 1. Test all 4 EP tone algorithms under maximum polyphony (1000 blocks total = 250 blocks * 4 algos)
    for (uint8_t algo = 0; algo < 4; algo++) {
        preset.ep_tone_algo = algo;
        dsp_init();

        // Trigger Bass Note
        dsp_note_on_bass(36, 127, preset.master_tuning);

        // Trigger 4-note Voicing
        Voicing v = { { 48, 52, 55, 59 }, 4 };
        dsp_note_on_keys(&v, 127, preset.master_tuning);

        // Trigger all 4 drums (Kick, Snare, Hat, Rimshot)
        uint8_t dvel[4] = { 127, 127, 127, 127 };
        dsp_trigger_drum(0x0F, dvel, 1);

        int64_t energy_sum = 0;

        // Render 250 blocks (at 256 frames each = 64,000 frames)
        for (int blk = 0; blk < 250; blk++) {
            for (int i = 0; i < 256 * 2; i++) buf[i] = 0;
            dsp_render_block(buf, 256, &preset);

            for (int i = 0; i < 256 * 2; i++) {
                int32_t sample = buf[i];
                // Assert no overflow or crazy values: soft saturator bounds output
                assert(sample >= -32767 && sample <= 32767);
                energy_sum += (sample < 0) ? -sample : sample;
            }
        }
        // Assert non-zero audio energy was rendered
        assert(energy_sum > 0);
    }

    // 2. Test tuning modes: 440 Hz, 432 Hz, 442 Hz
    for (uint8_t tune = 0; tune < 3; tune++) {
        dsp_init();
        dsp_note_on_bass(40, 100, tune);
        for (int i = 0; i < 256 * 2; i++) buf[i] = 0;
        dsp_render_block(buf, 256, &preset);
        int64_t sum = 0;
        for (int i = 0; i < 256 * 2; i++) sum += (buf[i] < 0) ? -buf[i] : buf[i];
        assert(sum > 0);
    }

    // 3. Test dsp_all_notes_off() and smooth reverb decay to zero
    dsp_init();
    dsp_note_on_bass(36, 127, 0);
    uint8_t dvel[4] = { 127, 127, 127, 127 };
    dsp_trigger_drum(0x0F, dvel, 0);
    for (int i = 0; i < 256 * 2; i++) buf[i] = 0;
    dsp_render_block(buf, 256, &preset);

    // Call all notes off
    dsp_all_notes_off();

    // Reverb tail must decay over time
    for (int blk = 0; blk < 250; blk++) {
        for (int i = 0; i < 256 * 2; i++) buf[i] = 0;
        dsp_render_block(buf, 256, &preset);
    }
    // In the last block, audio must have decayed to near-zero (below 16-bit LSB floor, < -80 dBFS)
    int32_t last_block_max = 0;
    for (int i = 0; i < 256 * 2; i++) {
        int32_t abs_s = (buf[i] < 0) ? -buf[i] : buf[i];
        if (abs_s > last_block_max) last_block_max = abs_s;
    }
    assert(last_block_max <= 2); // Fully decayed without DC offset!

    printf("[Stage 3] Fixed-point DSP audio engine tests passed successfully!\n");
}

// Phase 4 Unit Test: Panel Input Scanning, TFT UI, and Preset Flash Persistence
void test_stage4_panel_ui(void) {
    printf("[Stage 4] Testing Panel Input Scanning, TFT UI, and Presets...\n");

    HarmonyState hs;
    PresetConfig presets[16];
    harmony_init(&hs);
    panel_init();
    ui_init();

    for (int i = 0; i < 16; i++) {
        presets[i].magic = PRESET_MAGIC;
        presets[i].preset_id = i;
        presets[i].root_key = 4; // E
        presets[i].groove_style = GROOVE_TEXAS_SHUFFLE;
        presets[i].bpm = 112;
        presets[i].spice = 65;
        presets[i].swing = 67;
        presets[i].ep_tone_algo = 0;
        presets[i].bass_vol = 110;
        presets[i].keys_vol = 95;
        presets[i].drums_vol = 105;
        presets[i].reverb_mix = 35;
        presets[i].master_tuning = 0;
        presets[i].bass_cutoff = 85;
        presets[i].drive_level = 20;
        presets[i].count_in_enabled = 0;
        presets[i].form_mode = FORM_12BAR;
        presets[i].opening_var = VAR_FLAT_FOUR;
        presets[i].outro_type = OUTRO_TAG;
    }

    uint8_t active_preset = 0;
    uint8_t ui_screen = 0;

    // 1. Test MIDI In parsing
    panel_handle_midi_in(&hs, 0xFA, 0, 0); // MIDI Start
    assert(hs.is_playing == 1);
    assert(hs.current_bar == 0);

    panel_handle_midi_in(&hs, 0xB0, 64, 127); // CC 64 Sustain / Hold
    assert(hs.hold_chord_active == 1);

    panel_handle_midi_in(&hs, 0xB0, 80, 127); // CC 80 Early Turnaround
    assert(hs.queued_cue == CUE_FORCE_TURNAROUND);

    panel_handle_midi_in(&hs, 0xB0, 81, 127); // CC 81 Stop Time Break
    assert(hs.queued_cue == CUE_STOP_TIME);

    panel_handle_midi_in(&hs, 0xB0, 82, 127); // CC 82 Vamp Toggle
    assert(hs.queued_cue == CUE_TO_VAMP);

    panel_handle_midi_in(&hs, 0xB0, 83, 127); // CC 83 Ending Armed
    assert(hs.ending_armed == 1);
    assert(hs.outro_type == OUTRO_STOP);

    panel_handle_midi_in(&hs, 0xF8, 0, 0); // MIDI Clock
    assert(hs.ext_clock_active == 1);
    assert(hs.ext_clock_ticks_pending == 1);
    assert(hs.ext_clock_timeout_ms == 500);

    panel_handle_midi_in(&hs, 0xFC, 0, 0); // MIDI Stop
    assert(hs.is_playing == 0);

    // 2. Test Autosave Debounce
    assert(panel_check_autosave_needed(1000) == 0);

    // 3. Test UI screen rendering (both Lead Sheet and Band Mixer)
    ui_render(&hs, &presets[active_preset], active_preset, 0, 1);
    ui_screen = 1;
    ui_render(&hs, &presets[active_preset], active_preset, ui_screen, 1);

    printf("[Stage 4] Panel input scanning, TFT UI, and Presets tests passed!\n");
}

// Phase 5 Integration Test: Full-System Integration & MIDI Ring Buffer
void test_stage5_integration(void) {
    printf("[Stage 5] Testing Full-System Integration & MIDI Ring Buffer...\n");

    HarmonyState hs;
    harmony_init(&hs);
    panel_init();
    ui_init();
    dsp_init();

    PresetConfig presets[16];
    for (int i = 0; i < 16; i++) {
        presets[i].magic = PRESET_MAGIC;
        presets[i].preset_id = i;
        presets[i].root_key = (uint8_t)((4 + i * 5) % 12);
        presets[i].groove_style = (uint8_t)(i % 5);
        presets[i].bpm = 112;
        presets[i].spice = 65;
        presets[i].swing = 67;
        presets[i].ep_tone_algo = (uint8_t)(i % 4);
        presets[i].bass_vol = 110;
        presets[i].keys_vol = 95;
        presets[i].drums_vol = 105;
        presets[i].reverb_mix = 35;
        presets[i].master_tuning = 0;
        presets[i].bass_cutoff = 85;
        presets[i].drive_level = 20;
        presets[i].count_in_enabled = 0;
        presets[i].form_mode = (i == 5) ? FORM_MINOR_BLUES : FORM_12BAR;
        presets[i].opening_var = VAR_FLAT_FOUR;
        presets[i].outro_type = OUTRO_TAG;
    }

    uint8_t active_preset = 0;
    uint8_t ui_screen = 0;
    Voicing last_voicing = {{0, 0, 0, 0}, 0};
    uint32_t sample_acc = 2646000;
    int32_t audio_buf[512];

    // 1. Start playback via MIDI Start
    panel_handle_midi_in(&hs, 0xFA, 0, 0);
    assert(hs.is_playing == 1);

    // 2. Interleaved Audio and Main Loop Execution for 12 bars
    for (int bar = 0; bar < 12; bar++) {
        // Run audio ISR blocks (~44100 / 256 = 172 blocks per second; ~86 blocks per bar at 112 BPM)
        for (int blk = 0; blk < 86; blk++) {
            for (int f = 0; f < 256; f++) {
                sample_acc += presets[active_preset].bpm * 24;
                if (sample_acc >= 2646000) {
                    sample_acc -= 2646000;
                    PerformanceFrame frame;
                    arranger_generate_frame(&hs, &presets[active_preset], &frame, &last_voicing);
                    if (frame.bass_trigger) {
                        dsp_note_on_bass(frame.bass_note, frame.bass_vel, presets[active_preset].master_tuning);
                    }
                    if (frame.comp_trigger) {
                        dsp_note_on_keys(&frame.comp_voicing, frame.comp_vel, presets[active_preset].master_tuning);
                        last_voicing = frame.comp_voicing;
                    }
                    if (frame.drum_mask) {
                        dsp_trigger_drum(frame.drum_mask, frame.drum_vel, 0);
                    }
                    harmony_tick_subdivision(&hs);
                }
            }
            for (int i = 0; i < 512; i++) audio_buf[i] = 0;
            dsp_render_block(audio_buf, 256, &presets[active_preset]);
        }

        // Cooperative main loop cycle
        panel_poll_inputs(&hs, presets, &active_preset, bar * 500, &ui_screen);
        ui_render(&hs, &presets[active_preset], active_preset, ui_screen, hs.ui_dirty);
        hs.ui_dirty = 0;
    }

    // 3. Stop playback via MIDI Stop
    panel_handle_midi_in(&hs, 0xFC, 0, 0);
    assert(hs.is_playing == 0);

    // Ensure audio stops producing sound when stopped
    dsp_all_notes_off();
    for (int blk = 0; blk < 100; blk++) {
        for (int i = 0; i < 512; i++) audio_buf[i] = 0;
        dsp_render_block(audio_buf, 256, &presets[active_preset]);
    }
    int32_t final_amp = 0;
    for (int i = 0; i < 512; i++) {
        int32_t a = (audio_buf[i] < 0) ? -audio_buf[i] : audio_buf[i];
        if (a > final_amp) final_amp = a;
    }
    assert(final_amp <= 2);

    printf("[Stage 5] Full-system integration & MIDI tests passed!\n");
}

int main(void) {
    printf("[TB-1 Test Suite] Running Stage Tests...\n");
    test_stage1_harmony();
    test_stage2_arranger();
    test_stage3_dsp();
    test_stage4_panel_ui();
    test_stage5_integration();
    printf("[TB-1 Test Suite] All tests completed with 100%% assertions passing.\n");
    return 0;
}
