#include <stdio.h>
#include <assert.h>
#include "../firmware/src/blues_types.h"
#include "../firmware/src/blues_harmony.h"
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

int main(void) {
    printf("[TB-1 Test Suite] Running Phase 1 Stage Tests...\n");
    test_stage1_harmony();
    printf("[TB-1 Test Suite] All tests completed with 100%% assertions passing.\n");
    return 0;
}
