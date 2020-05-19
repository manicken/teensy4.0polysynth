#pragma once
#include <Audio.h>

extern const AudioSynthWavetable::sample_data GrandPiano_samples[5];
const uint8_t GrandPiano_ranges[] = {42, 53, 65, 77, 127, };

const AudioSynthWavetable::instrument_data GrandPiano = {5, GrandPiano_ranges, GrandPiano_samples };

extern const uint32_t sample_4_GrandPiano_pfd1[23424];

extern const uint32_t sample_0_GrandPiano_pfc2[22784];

extern const uint32_t sample_1_GrandPiano_pfc3[23040];

extern const uint32_t sample_2_GrandPiano_pfc4[23168];

extern const uint32_t sample_3_GrandPiano_pfc5[23040];


