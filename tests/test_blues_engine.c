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
        FORM_8BAR, FORM_MINOR_BLUES, FORM_I_bVII_IV
    };
    for (int f = 0; f < 6; f++) {
        hs.form_mode = forms[f];
        hs.chorus_count = 1; // Test standard form (past intro)
        uint8_t max_b = (forms[f] == FORM_8BAR || forms[f] == FORM_I_bVII_IV) ? 8 : 12;
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

    hs.form_mode = FORM_I_bVII_IV;
    hs.current_bar = 0;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_I7);
    hs.current_bar = 1;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_bVII7);
    hs.current_bar = 2;
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_IV7);

    // 5. Test Opening Variations (Chorus 0 only)
    hs.form_mode = FORM_12BAR;
    hs.variation = 0;

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

    // 7. Test Outro Cadences and Termination (all 5 must transition next bar and stop)
    OutroType outros[5] = { OUTRO_CLASSIC_SLIDE, OUTRO_V_bV_IV_I, OUTRO_IV_I_TURN, OUTRO_QUICK_1BAR, OUTRO_STINGER_ONLY };
    for (int o = 0; o < 5; o++) {
        harmony_init(&hs);
        hs.is_playing = 1;
        hs.outro_type = outros[o];
        hs.current_bar = 3; // Mid-chorus bar!
        hs.ending_armed = ENDING_ARMED_NEXT_BAR;
        seq_advance_bar(&hs);
        if (outros[o] == OUTRO_STINGER_ONLY) {
            assert(hs.ending_armed == ENDING_STINGER);
        } else {
            if (outros[o] == OUTRO_QUICK_1BAR) {
                assert(hs.ending_armed == ENDING_BAR_2);
            } else {
                assert(hs.ending_armed == ENDING_BAR_1);
                BarSlot s1 = harmony_get_current_bar_slot(&hs);
                assert(s1.primary <= DEG_SHARP_IV_DIM7);
                seq_advance_bar(&hs);
                assert(hs.ending_armed == ENDING_BAR_2);
            }
            BarSlot s2 = harmony_get_current_bar_slot(&hs);
            assert(s2.primary <= DEG_SHARP_IV_DIM7);
            // Next bar -> enters final Beat-1 Root stinger (ENDING_STINGER)
            seq_advance_bar(&hs);
            assert(hs.ending_armed == ENDING_STINGER);
        }
        // If seq_advance_bar runs again without arranger, it also stops cleanly
        seq_advance_bar(&hs);
        assert(hs.is_playing == 0);
        assert(hs.ending_armed == ENDING_NONE);
    }

    // Verify arranger_generate_frame plays final Root (I9) hit on ENDING_STINGER and stops
    harmony_init(&hs);
    hs.is_playing = 1;
    hs.ending_armed = ENDING_STINGER;
    hs.root_semitone = 4; // E
    hs.current_bar = 0;
    hs.current_beat = 0;
    hs.current_subdiv = 0;
    PresetConfig end_p;
    memset(&end_p, 0, sizeof(end_p));
    PerformanceFrame end_f;
    Voicing end_v = { {0}, 0 };
    arranger_generate_frame(&hs, &end_p, &end_f, &end_v);
    assert(end_f.bass_trigger == 1);
    assert(end_f.bass_note == 40); // E2 root
    assert(end_f.comp_trigger == 1);
    assert(end_f.comp_voicing.count == 4);
    assert(end_f.drum_mask == 0x07);
    assert(hs.is_playing == 0);
    assert(hs.ending_armed == ENDING_NONE);

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
        assert(v_shell.notes[i] >= 56 && v_shell.notes[i] <= 90);
    }

    // Spice 40 -> 3-note voicing
    Voicing v_3 = voice_lead_select(prev, DEG_I7, 4, 40);
    assert(v_3.count == 3);

    // Spice 65 -> 4-note voicing
    Voicing v_i7 = voice_lead_select(prev, DEG_I7, 4, 65);
    assert(v_i7.count == 4);
    for (int i = 0; i < v_i7.count; i++) {
        assert(v_i7.notes[i] >= 56 && v_i7.notes[i] <= 90);
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
    preset.variation = 65;
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
    hs.active_intro_style = INTRO_CLICK_1BAR;
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
    preset.variation = 65;
    preset.bass_vol = 127;
    preset.keys_vol = 127;
    preset.drums_vol = 127;
    preset.reverb_mix = 127;
    preset.drive_level = 100;
    preset.bass_cutoff = 100;
    preset.master_tuning = 0; // 440 Hz
    preset.ep_tone_algo = 0;
    preset.bass_instrument = 0;
    preset.drum_kit = 0;
    preset.lead_instrument = 0;
    preset.lead_vol = 115;

    int32_t buf[256 * 2];

    // 1. Test all 8 EP tone algorithms, 4 Bass engines, 3 Drum kits, and 4 Lead instruments
    for (uint8_t algo = 0; algo < 8; algo++) {
        preset.ep_tone_algo = algo;
        preset.bass_instrument = algo % 4;
        preset.drum_kit = algo % 3;
        preset.lead_instrument = algo % 4;
        dsp_init();

        // Trigger Bass Note
        dsp_note_on_bass(36, 127, preset.master_tuning);

        // Trigger 4-note Voicing
        Voicing v = { { 48, 52, 55, 59 }, 4 };
        dsp_note_on_keys(&v, 127, preset.master_tuning);

        // Trigger Solo Lead Note
        dsp_note_on_lead(65 + algo, 115, preset.master_tuning);

        // Trigger all 4 drums (Kick, Snare, Hat, Rimshot)
        uint8_t dvel[NUM_DRUM_VOICES] = { 127, 127, 127, 127 };
        dsp_trigger_drum(0x0F, dvel);

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
    uint8_t dvel[NUM_DRUM_VOICES] = { 127, 127, 127, 127 };
    dsp_trigger_drum(0x0F, dvel);
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
        presets[i].variation = 65;
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
        presets[i].outro_type = OUTRO_CLASSIC_SLIDE;
        presets[i].bass_instrument = (uint8_t)(i % 4);
        presets[i].drum_kit = (uint8_t)(i % 4);
        presets[i].lead_instrument = (uint8_t)(i % 8);
        presets[i].lead_vol = 115;
    }

    uint8_t active_preset = 0;
    uint8_t ui_screen = 0;

    // 1. Test MIDI In parsing
    panel_handle_midi_in(&hs, 0xFA, 0, 0); // MIDI Start
    assert(hs.is_playing == 1);
    assert(hs.current_bar == 0);

    panel_handle_midi_in(&hs, 0xB0, 64, 127); // CC 64 Sustain / Hold
    assert(hs.hold_chord_active == 1);

    panel_handle_midi_in(&hs, 0xB0, 80, 127); // Footswitch A while playing -> Arm Next-Bar Outro
    assert(hs.ending_armed == ENDING_ARMED_NEXT_BAR);
    hs.ending_armed = ENDING_NONE;

    panel_handle_midi_in(&hs, 0xB0, 81, 127); // Footswitch B while playing -> Vamp Toggle
    assert(hs.queued_cue == CUE_TO_VAMP);
    hs.queued_cue = CUE_NONE;

    panel_handle_midi_in(&hs, 0xB0, 82, 127); // Footswitch C while playing -> Heat Down 3->2
    assert(hs.heat_level == 2);
    assert(hs.fill_pending == 0);

    panel_handle_midi_in(&hs, 0xB0, 83, 127); // Footswitch D while playing -> Heat Up 2->3
    assert(hs.heat_level == 3);
    panel_handle_midi_in(&hs, 0xB0, 83, 127); // Second press -> Heat Up 3->4 (no drum fill)
    assert(hs.heat_level == 4);
    assert(hs.fill_pending == 0);

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
        presets[i].variation = 65;
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
        presets[i].outro_type = OUTRO_CLASSIC_SLIDE;
        presets[i].bass_instrument = (uint8_t)(i % 4);
        presets[i].drum_kit = (uint8_t)(i % 4);
        presets[i].lead_instrument = (uint8_t)(i % 8);
        presets[i].lead_vol = 115;
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
                        dsp_trigger_drum(frame.drum_mask, frame.drum_vel);
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

    // 4. Verify Fullness (sparse vs full) & Bar 12 phrase-end fill
    HarmonyState fhs;
    harmony_init(&fhs);
    fhs.is_playing = 1;
    fhs.current_bar = 1;
    fhs.current_beat = 1;
    fhs.current_subdiv = 0;
    PresetConfig fp = presets[0];
    Voicing fv = { {0}, 0 };
    PerformanceFrame pf;

    fhs.bass_fullness = 10;  // Ultra-sparse: no beat-1 bass, beat-2 plays 5th (1-5-1-5, never 1-6-1-6)
    fhs.drums_fullness = 10; // Ultra-sparse: rimshot instead of snare
    fhs.bass_variation = 80;
    arranger_generate_frame(&fhs, &fp, &pf, &fv);
    assert(pf.bass_trigger == 0);
    assert((pf.drum_mask & 0x08) != 0);
    assert((pf.drum_mask & 0x02) == 0);

    // Beat 2 at lowest bass_fullness (< 20) plays 1 note per measure (Beat 0 only)
    fhs.current_bar = 0;
    fhs.current_beat = 2;
    arranger_generate_frame(&fhs, &fp, &pf, &fv);
    assert(pf.bass_trigger == 0);

    // Beat 2 at bass_fullness 20..39 plays 5th (+7 = 47 for E2=40), NOT 6th (+9 = 49)
    fhs.bass_fullness = 25;
    arranger_generate_frame(&fhs, &fp, &pf, &fv);
    assert(pf.bass_trigger == 1);
    assert(pf.bass_note == 47);

    fhs.bass_fullness = 95;
    fhs.drums_fullness = 95; // Full rocking band + Bar 12 beat 3 offbeat fill
    fhs.drums_variation = 80;
    fhs.current_bar = 11;
    fhs.current_beat = 3;
    fhs.current_subdiv = 16; // offbeat_subdiv for swing 67 = 12 + (67*6)/100 = 16
    arranger_generate_frame(&fhs, &fp, &pf, &fv);
    // 2.3: the chorus ends on one of several fills (blues_patterns.c): its last step is a snare or a tom, and no hat
    assert((pf.drum_mask & ((1u << DV_SNARE) | (1u << DV_TOM_HI) | (1u << DV_TOM_LO))) != 0);
    assert((pf.drum_mask & (1u << DV_HAT)) == 0);

    // Expanded Styles & Sounds verification
    for (uint8_t dp = 0; dp < NUM_DRUM_PATTERNS; dp++) {
        arranger_apply_groove_defaults(&fhs, &fp, (GrooveStyle)dp);
        fhs.current_bar = 0; fhs.current_beat = 0; fhs.current_subdiv = 0;
        arranger_generate_frame(&fhs, &fp, &pf, &fv);
        assert(pf.drum_mask != 0);
    }

    int32_t test_buf[64];
    for (uint8_t li = 0; li < NUM_LEAD_INSTRUMENTS; li++) {
        fp.lead_instrument = li;
        fp.lead_play_style = (uint8_t)(li % NUM_LEAD_PLAY_STYLES);
        dsp_set_lead_play_style(fp.lead_play_style);
        dsp_note_on_lead(69, 110, 0);
        dsp_render_block(test_buf, 64, &fp);
        int32_t energy = 0;
        for (int i = 0; i < 64; i++) energy += (test_buf[i] < 0) ? -test_buf[i] : test_buf[i];
        assert(energy > 0);
        dsp_note_off_lead(69);
    }

    // Verify Easy Lead polyphonic unlit-key bends and splash screen render
    dsp_set_lead_play_style(LEAD_PLAY_POLY);
    dsp_note_on_lead_ex(58, 59, 115, 0);
    dsp_note_on_lead_ex(61, 63, 115, 0);
    dsp_render_block(test_buf, 64, &fp);
    dsp_note_off_lead(58);
    dsp_note_off_lead(61);
    ui_render_splash();

    printf("[Stage 5] Full-system integration & MIDI tests passed!\n");
}

void test_stage6_v2_features(void) {
    printf("[Stage 6] Testing V2 Footswitch, Musical Lead-Ins, Next-Bar Outro & Riffs...\n");
    HarmonyState hs;
    PresetConfig p;
    memset(&p, 0, sizeof(p));
    p.bpm = 120;
    p.swing = 67;
    p.bass_vol = 96;
    p.keys_vol = 88;
    p.drums_vol = 96;
    p.lead_vol = 116;

    // 1. M-VAVE Chocolate Program Change (PC 0..3) while stopped -> Start, Intro Start, Cycle Intro, Cycle Outro
    harmony_init(&hs);
    assert(hs.count_in_enabled == INTRO_AUTO);
    assert(hs.outro_type == OUTRO_CLASSIC_SLIDE);

    // PC 1 (Switch B) while stopped cycles Intro mode (INTRO_AUTO -> INTRO_V7_1BAR)
    panel_handle_midi_in(&hs, 0xC0, 1, 0);
    assert(hs.count_in_enabled == INTRO_V7_1BAR);
    // Cycle back to INTRO_AUTO
    hs.count_in_enabled = INTRO_AUTO;
    // PC 2 (Switch C) / PC 3 (Switch D) while stopped step Heat Down/Up
    panel_handle_midi_in(&hs, 0xC0, 2, 0);
    assert(hs.heat_level == 2);
    panel_handle_midi_in(&hs, 0xC0, 3, 0);
    assert(hs.heat_level == 3);
    // PC 0 starts playback with Intro armed
    panel_handle_midi_in(&hs, 0xC0, 0, 0);
    assert(hs.is_playing == 1);
    assert(hs.count_in_bars_remaining == 2); // TEXAS SHUFFLE -> INTRO_TURNAROUND_2BAR (2 bars)
    // PC 0 while playing arms Next-Bar Outro
    panel_handle_midi_in(&hs, 0xC0, 0, 0);
    assert(hs.ending_armed == ENDING_ARMED_NEXT_BAR);
    // PC 0 again during armed outro hard-stops
    panel_handle_midi_in(&hs, 0xC0, 0, 0);
    assert(hs.is_playing == 0);
    assert(hs.ending_armed == ENDING_NONE);

    // 2. Chocolate Switch D (Note 65 / CC 23 / PC 3) & Switch C (Note 64 / CC 22 / PC 2) while stopped -> Clamped Heat
    harmony_init(&hs);
    assert(hs.heat_level == 3);
    panel_handle_midi_in(&hs, 0x90, 65, 127); // Note 65 (Sw D): 3 -> 4
    assert(hs.heat_level == 4 && hs.is_playing == 0);
    panel_handle_midi_in(&hs, 0xB0, 23, 127); // CC 23 (Sw D): 4 -> 5
    assert(hs.heat_level == 5);
    panel_handle_midi_in(&hs, 0xC0, 3, 0);    // PC 3 (Sw D): clamped at 5
    assert(hs.heat_level == 5);
    panel_handle_midi_in(&hs, 0x90, 64, 127); // Note 64 (Sw C): 5 -> 4
    assert(hs.heat_level == 4);
    harmony_set_heat_level(&hs, &p, 1, 0);
    panel_handle_midi_in(&hs, 0xB0, 22, 127); // CC 22 (Sw C): clamped at 1
    assert(hs.heat_level == 1 && hs.fullness == 25 && hs.variation == 15);

    // 3. Verify all Intro styles emit Bass + Keys + Drum notes while keeping current_bar == 0
    IntroMode intro_modes[6] = { INTRO_V7_1BAR, INTRO_TURNAROUND_2BAR, INTRO_FUNK_2BAR, INTRO_BAND_4BAR, INTRO_CLICK_1BAR, INTRO_DRONE_1BAR };
    uint8_t expected_bars[6] = { 1, 2, 2, 4, 1, 1 };
    for (int m = 0; m < 6; m++) {
        harmony_init(&hs);
        hs.count_in_enabled = (uint8_t)intro_modes[m];
        harmony_start_playback(&hs, GROOVE_TEXAS_SHUFFLE, 0);
        assert(hs.count_in_bars_remaining == expected_bars[m]);
        int bass_hits = 0, keys_hits = 0, drum_hits = 0;
        Voicing v = { {0}, 0 };
        for (int b = 0; b < expected_bars[m]; b++) {
            assert(hs.current_bar == 0);
            for (int beat = 0; beat < 4; beat++) {
                for (int sub = 0; sub < 24; sub++) {
                    hs.current_beat = (uint8_t)beat;
                    hs.current_subdiv = (uint8_t)sub;
                    PerformanceFrame f;
                    arranger_generate_frame(&hs, &p, &f, &v);
                    if (f.bass_trigger) bass_hits++;
                    if (f.comp_trigger) keys_hits++;
                    if (f.drum_mask) drum_hits++;
                }
            }
            seq_advance_bar(&hs);
        }
        assert(hs.count_in_bars_remaining == 0);
        assert(hs.current_bar == 0); // Now lands cleanly on Chorus 1 Bar 1!
        assert(drum_hits >= 4);
        if (intro_modes[m] == INTRO_DRONE_1BAR) {
            assert(bass_hits == 1);
            assert(keys_hits == 1);
        } else if (intro_modes[m] != INTRO_CLICK_1BAR) {
            assert(bass_hits >= 4);
            assert(keys_hits >= 3);
        }
    }

    // 4. Verify Classic Slide Outro #I9 -> I9 half-step slide-down from mid-chorus (Bar 4)
    harmony_init(&hs);
    hs.is_playing = 1;
    hs.root_semitone = 4; // E (bass root = 40, keys root = 64)
    hs.current_bar = 4;
    hs.outro_type = OUTRO_CLASSIC_SLIDE;
    hs.ending_armed = ENDING_ARMED_NEXT_BAR;
    seq_advance_bar(&hs);
    assert(hs.ending_armed == ENDING_BAR_1);
    seq_advance_bar(&hs);
    assert(hs.ending_armed == ENDING_BAR_2);

    // Check Beat 4& (subdiv 16 for swing 67 -> step16 = 14) emits #I9 (bass=41, keys[0]=65)
    Voicing ov = { {0}, 0 };
    PerformanceFrame of;
    hs.current_beat = 3;
    hs.current_subdiv = 16;
    arranger_generate_frame(&hs, &p, &of, &ov);
    assert(of.bass_trigger == 1 && of.bass_note == 41); // F2 (#I)
    assert(of.comp_trigger == 1 && of.comp_voicing.notes[0] == 65); // F4 (#I9)

    // Advance to ENDING_STINGER -> Beat 1 emits I9 (bass=40, keys[0]=64) and stops
    seq_advance_bar(&hs);
    assert(hs.ending_armed == ENDING_STINGER);
    hs.current_beat = 0;
    hs.current_subdiv = 0;
    arranger_generate_frame(&hs, &p, &of, &ov);
    assert(of.bass_trigger == 1 && of.bass_note == 40); // E2 (I)
    assert(of.comp_trigger == 1 && of.comp_voicing.notes[0] == 64); // E4 (I9)
    assert(hs.is_playing == 0);
    assert(hs.ending_armed == ENDING_NONE);

    // 5. Render Lead Sheet & Style screens cleanly
    ui_init();
    ui_render(&hs, &p, 0, 0, 1);
    ui_render(&hs, &p, 0, 2, 1);
    printf("[Stage 6] V2 Footswitch, Lead-Ins, Next-Bar Outro & Riffs passed!\n");
}

void test_stage7_escalation_heat(void) {
    printf("[Stage 7] Testing 5-Level Jam Escalation (Band Heat) & Stage Visuals...\n");
    HarmonyState hs;
    PresetConfig p;
    memset(&p, 0, sizeof(p));
    p.bpm = 112;
    p.bass_vol = 96;
    p.keys_vol = 88;
    p.drums_vol = 96;
    p.lead_vol = 116;
    p.lead_instrument = LEAD_ALTO_SAX;

    harmony_init(&hs);
    assert(hs.heat_level == 3);

    static const uint8_t EXP_FUL[5] = { 25, 45, 65, 80, 100 };
    static const uint8_t EXP_VAR[5] = { 15, 30, 50, 75, 100 };
    for (uint8_t lvl = 1; lvl <= 5; lvl++) {
        harmony_set_heat_level(&hs, &p, lvl, 0);
        assert(hs.heat_level == lvl);
        assert(p.heat_level == lvl);
        assert(hs.fullness == EXP_FUL[lvl - 1]);
        assert(hs.variation == EXP_VAR[lvl - 1]);
    }

    // Verify Switch D (Heat Up, clamped 5) & Switch C (Heat Down, clamped 1) with no drum fill, plus Switch B (Vamp)
    hs.is_playing = 1;
    hs.fill_pending = 0;
    harmony_set_heat_level(&hs, &p, 5, 0);
    panel_handle_midi_in(&hs, 0xC0, 3, 0); // Switch D at 5 -> stays clamped at 5
    assert(hs.heat_level == 5);
    assert(hs.fill_pending == 0);
    panel_handle_midi_in(&hs, 0xC0, 2, 0); // Switch C: 5 -> 4
    assert(hs.heat_level == 4);
    assert(hs.fill_pending == 0);
    harmony_set_heat_level(&hs, &p, 1, 0);
    panel_handle_midi_in(&hs, 0xC0, 2, 0); // Switch C at 1 -> stays clamped at 1
    assert(hs.heat_level == 1);
    panel_handle_midi_in(&hs, 0xC0, 3, 0); // Switch D: 1 -> 2
    assert(hs.heat_level == 2);
    assert(hs.fill_pending == 0);

    // Verify Minor Blues i-iv Vamp (2 bars i7, 2 bars iv7) via Switch B
    hs.form_mode = FORM_MINOR_BLUES;
    hs.queued_cue = CUE_NONE;
    panel_handle_midi_in(&hs, 0xC0, 1, 0); // Switch B: queue Vamp
    assert(hs.queued_cue == CUE_TO_VAMP);
    seq_advance_bar(&hs);
    assert(hs.is_vamp_mode == 1 && hs.vamp_bar_counter == 0);
    assert(harmony_get_current_bar_slot(&hs).primary == DEG_i7);
    seq_advance_bar(&hs);
    assert(hs.vamp_bar_counter == 1 && harmony_get_current_bar_slot(&hs).primary == DEG_i7);
    seq_advance_bar(&hs);
    assert(hs.vamp_bar_counter == 2 && harmony_get_current_bar_slot(&hs).primary == DEG_iv7);
    seq_advance_bar(&hs);
    assert(hs.vamp_bar_counter == 3 && harmony_get_current_bar_slot(&hs).primary == DEG_iv7);
    hs.is_vamp_mode = 0;
    hs.form_mode = FORM_12BAR;

    // Verify DSP volume escalation (Level 1 < Level 3 < Level 5)
    dsp_init();
    int32_t buf1[128], buf3[128], buf5[128];
    memset(buf1, 0, sizeof(buf1));
    memset(buf3, 0, sizeof(buf3));
    memset(buf5, 0, sizeof(buf5));
    uint64_t e1 = 0, e3 = 0, e5 = 0;

    p.heat_level = 1;
    dsp_note_on_bass(40, 120, 0);
    dsp_render_block(buf1, 64, &p);
    for (int i = 0; i < 64; i++) e1 += (uint64_t)(buf1[i] < 0 ? -buf1[i] : buf1[i]);

    dsp_init();
    p.heat_level = 3;
    dsp_note_on_bass(40, 120, 0);
    dsp_render_block(buf3, 64, &p);
    for (int i = 0; i < 64; i++) e3 += (uint64_t)(buf3[i] < 0 ? -buf3[i] : buf3[i]);

    dsp_init();
    p.heat_level = 5;
    dsp_note_on_bass(40, 120, 0);
    dsp_render_block(buf5, 64, &p);
    for (int i = 0; i < 64; i++) e5 += (uint64_t)(buf5[i] < 0 ? -buf5[i] : buf5[i]);

    assert(e1 > 0 && e1 < e3 && e3 < e5);

    // Verify UI stage rendering across all 5 Heat levels
    ui_init();
    hs.vis_bass_pulse = 2;
    hs.vis_keys_pulse = 2;
    hs.vis_drum_pulse = 2;
    for (uint8_t lvl = 1; lvl <= 5; lvl++) {
        hs.heat_level = lvl;
        ui_render(&hs, &p, 0, 0, 1);
    }

    // Verify FORM_I_bVII_IV (8-bar I-bVII-IV-I) and GROOVE_SWAMP_ROCK
    harmony_init(&hs);
    arranger_apply_groove_defaults(&hs, &p, GROOVE_SWAMP_ROCK);
    assert(p.groove_style == GROOVE_SWAMP_ROCK && hs.drum_pattern == GROOVE_SWAMP_ROCK);
    hs.form_mode = FORM_I_bVII_IV;
    static const ScaleDegree EXP_RGT[8] = {
        DEG_I7, DEG_bVII7, DEG_IV7, DEG_I7, DEG_I7, DEG_bVII7, DEG_IV7, DEG_I7
    };
    for (uint8_t b = 0; b < 8; b++) {
        assert(hs.current_bar == b);
        assert(harmony_get_current_bar_slot(&hs).primary == EXP_RGT[b]);
        seq_advance_bar(&hs);
    }
    assert(hs.current_bar == 0 && hs.chorus_count == 1);

    printf("[Stage 7] 5-Level Jam Escalation & Stage Visuals passed!\n");
}

int main(void) {
    printf("[TB-1 Test Suite] Running Stage Tests...\n");
    test_stage1_harmony();
    test_stage2_arranger();
    test_stage3_dsp();
    test_stage4_panel_ui();
    test_stage5_integration();
    test_stage6_v2_features();
    test_stage7_escalation_heat();
    printf("[TB-1 Test Suite] All tests completed with 100%% assertions passing.\n");
    return 0;
}
