/* SPDX-License-Identifier: GPL-3.0-only */
/* A voice on its own, to measure (tests/voice_check.py): one note of a bass, keys or lead instrument, or one drum.
 *   voice_probe bass|keys|lead|drum INSTRUMENT NOTE SECONDS OUT.s16 [fx1 amt [slide]]
 * Build: cc -O1 -DSTANDALONE_TEST -Ifirmware/src -o build/host/voice_probe tests/voice_probe.c firmware/src/dsp_voices.c -lm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../firmware/src/dsp_voices.h"

int main(int argc, char **argv) {
    if (argc < 6) return 2;
    int inst = atoi(argv[2]), note = atoi(argv[3]);
    int n = (int)(atof(argv[4]) * 44100.0);
    PresetConfig p;
    memset(&p, 0, sizeof p);
    p.bass_vol = p.keys_vol = p.drums_vol = p.lead_vol = 127;
    p.bpm = 120;
    p.heat_level = 3;
    p.lead_fx1 = argc > 6 ? (uint8_t)atoi(argv[6]) : 0;
    p.lead_fx1_amt = argc > 7 ? (uint8_t)atoi(argv[7]) : 0;
    p.lead_slide = argc > 8 ? (uint8_t)atoi(argv[8]) : 0;
    char kind = argv[1][0];
    if (kind == 'b') p.bass_instrument = (uint8_t)inst;
    if (kind == 'k') p.ep_tone_algo = (uint8_t)inst;
    if (kind == 'l') p.lead_instrument = (uint8_t)inst;
    if (kind == 'd') p.drum_kit = (uint8_t)inst;
    dsp_init();
    int32_t buf[512];
    FILE *f = fopen(argv[5], "wb");
    if (!f) return 1;
    memset(buf, 0, sizeof buf);
    dsp_render_block(buf, 256, &p);                 /* (the engine learns the preset's instruments) */
    for (int done = 0, first = 1; done < n; done += 256) {
        if (first) {
            first = 0;
            if (kind == 'b') dsp_note_on_bass_ex((uint8_t)note, 110, 0, 44100, 0);
            if (kind == 'k') { Voicing v = { { (uint8_t)note, 0, 0, 0 }, 1 }; dsp_note_on_keys(&v, 110, 0); dsp_extend_drone_gate(44100); }
            if (kind == 'l') dsp_note_on_lead((uint8_t)note, 115, 0);
            if (kind == 'd') { uint8_t vel[NUM_DRUM_VOICES]; memset(vel, 110, sizeof vel); dsp_trigger_drum((uint16_t)(1u << note), vel); }
        }
        if (kind == 'l' && done >= n * 3 / 4) dsp_note_off_lead((uint8_t)note);
        memset(buf, 0, sizeof buf);
        dsp_render_block(buf, 256, &p);
        for (int i = 0; i < 256; i++) {
            int32_t s = buf[i * 2];
            int16_t o = (int16_t)(s > 32767 ? 32767 : s < -32767 ? -32767 : s);
            fwrite(&o, 2, 1, f);
        }
    }
    fclose(f);
    return 0;
}
