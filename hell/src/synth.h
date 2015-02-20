#pragma once


#include <stdint.h>


enum {
	MIXRATE = 48000,
	NUM_VOICES = 8,
	NUM_OSCS = 3,
	NUM_ENVS = 2,
	NUM_LFOS = 2,
};


enum { OSC_OFF, OSC_TRIANGLE, OSC_PULSE, OSC_SINE };
typedef struct {
	uint32_t	mode;
	float		transpose;
	float		detune;
	float		volume;
	float		pulsewidth;
} OscPatch;


typedef struct {
	float		attack;
	float		decay;
	float		sustain;
	float		release;
} EnvPatch;


typedef struct {
	uint32_t	mode;
	uint32_t	sync;
	float		rate;
	float		phase;
	float		amplify;
} LFOscPatch;


typedef struct {
	float		panning;
	OscPatch	oscs[NUM_OSCS];
	EnvPatch	envs[NUM_ENVS];
	LFOscPatch	lfos[NUM_LFOS];
} VoicePatch;


typedef struct {
	uint8_t		src_index;
	uint8_t		trg_index;
	float		gain;
} PatchCord;


typedef struct {
	VoicePatch	voice;
	uint8_t		num_cords;
	PatchCord	cords[256];
} SynthPatch;


enum {
	SRC_VELOCITY,
	SRC_ENV1,
	SRC_ENV2,
	SRC_LFO1,
	SRC_LFO2,
};


void synth_init(SynthPatch* patch);
void synth_event(uint8_t type, uint8_t a, uint8_t b);
void synth_mix(float out[2]);

