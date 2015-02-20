#include <stdio.h>
#include <string.h>
#include <math.h>
#include "synth.h"


#define M_PI	3.141592653589793


typedef struct {
	float	phase;
} Osc;


enum { ENV_OFF, ENV_ATTACK, ENV_HOLD, ENV_RELEASE };
typedef struct {
	uint8_t	state;
	float	level;
} Env;


typedef struct {
	float	phase;
	float	out;
} LFOsc;


typedef struct {
	uint8_t	note;
	float	velocity;
	Osc		oscs[NUM_OSCS];
	Env		envs[NUM_ENVS];
	LFOsc	lfos[NUM_LFOS];

	union {
		VoicePatch	patch;
		float		targets[0];
	};

} Voice;


static struct {
	SynthPatch*	patch;
	Voice		voices[NUM_VOICES];
} synth;




void synth_init(SynthPatch* patch) {
	synth.patch = patch;
}


static void voice_mix(Voice* voice, float out[2]) {
	if (voice->envs[0].state == ENV_OFF) return;

	float buf = 0;

	// oscs
	for (int i = 0; i < NUM_OSCS; i++) {
		if (voice->patch.oscs[i].mode == OSC_OFF) continue;

		float pitch = voice->patch.oscs[i].transpose * 12 +
					  voice->patch.oscs[i].detune +
					  voice->note - 57;


		float pp = voice->oscs[i].phase;

		voice->oscs[i].phase += 440.0 * exp2f(pitch / 12.0) / MIXRATE;
		voice->oscs[i].phase -= (int) voice->oscs[i].phase;

		float p = voice->oscs[i].phase;
		float pw = voice->patch.oscs[i].pulsewidth;

		float f;
		switch (voice->patch.oscs[i].mode) {
		case OSC_PULSE:
		default:
//*
			if (pp <= pw && p > pw)
				f = (pw - pp) * -1 + (p - pw) * 1;
			else if (p < pp)
				f = p * -1 + (1 - pp) * 1;
			else
				f = p < pw ? -1 : 1;
/*/
			f = voice->oscs[i].phase < voice->patch.oscs[i].pulsewidth ? -1 : 1;
//*/
			break;


		case OSC_SINE:
			f = sinf(voice->oscs[i].phase * 2 * M_PI);
			break;
		}

		buf += f * voice->patch.oscs[i].volume;
	}

	// envs
	for (int i = 0; i < NUM_ENVS; i++) {

		switch (voice->envs[i].state) {
		case ENV_OFF: continue;
		case ENV_ATTACK:
			voice->envs[i].level += 0.01;
			if (voice->envs[i].level > 1) {
				voice->envs[i].level = 1;
				voice->envs[i].state = ENV_HOLD;
			}
			break;
		case ENV_HOLD:
			voice->envs[i].level = 0.7 + (voice->envs[i].level - 0.7) * 0.9999;
			break;
		case ENV_RELEASE:
		default:
			voice->envs[i].level *= 0.9997;
			if (voice->envs[i].level < 0.0005) {
				voice->envs[i].state = ENV_OFF;
			}
			break;
		}

	}

	//printf("%f\n", voice->envs[0].level);

	buf *= expf((voice->velocity - 1) * 2);
	buf *= voice->envs[0].level;

	out[0] += buf * sqrtf(0.5 - voice->patch.panning * 0.5);
	out[1] += buf * sqrtf(0.5 + voice->patch.panning * 0.5);


	// lfos
	for (int i = 0; i < NUM_LFOS; i++) {
		float speed = exp2f(voice->patch.lfos[i].rate * 8 - 4) / MIXRATE;
		voice->lfos[i].phase += speed;
		voice->lfos[i].phase -= (int) voice->lfos[i].phase;

		float buf = sinf(voice->lfos[i].phase * M_PI * 2);
		voice->lfos[i].out = buf * voice->patch.lfos[i].amplify;
	}


}


void synth_mix(float out[2]) {

	out[0] = out[1] = 0;

	for (int v = 0; v < NUM_VOICES; v++) {
		Voice* voice = &synth.voices[v];

		float sources[] = {
			voice->velocity,
			voice->envs[0].level,
			voice->envs[1].level,
			voice->lfos[0].out,
			voice->lfos[1].out,
		};

		memcpy(&voice->patch, &synth.patch->voice, sizeof(VoicePatch));

		for (int p = 0; p < synth.patch->num_cords; p++) {
			PatchCord* cord = &synth.patch->cords[p];
			voice->targets[cord->trg_index] += sources[cord->src_index] * cord->gain;
		}

		voice_mix(voice, out);
	}

}


static float voice_get_business(const Voice* v) {
	if (v->envs[0].state == ENV_OFF) return 0;
	if (v->envs[0].state == ENV_RELEASE) return v->envs[0].level;
	return 2;
}


static void synth_note_on(uint8_t note, uint8_t velocity) {
	Voice* voice;
	float business = 10;
	for (int i = 0; i < NUM_VOICES; i++) {
		Voice* v = &synth.voices[i];
		float b = voice_get_business(v);
		if (b < business) {
			voice = v;
			if (b == 0) break;
			business = b;
		}
	}

	voice->note = note;
	voice->velocity = velocity / 127.0;

	for (int i = 0; i < NUM_OSCS; i++) {
		voice->oscs[i].phase = 0;
	}

	for (int i = 0; i < NUM_ENVS; i++) {
		voice->envs[i].state = ENV_ATTACK;
		voice->envs[i].level = 0;
	}

	for (int i = 0; i < NUM_LFOS; i++) {
		if (voice->patch.lfos[i].sync) {
			voice->lfos[i].phase = voice->patch.lfos[i].phase;
			// TODO: set out?
		}
	}
}


static void synth_note_off(uint8_t note) {
	Voice* voice = NULL;
	for (int i = 0; i < NUM_VOICES; i++) {
		voice = &synth.voices[i];
		if (voice->note == note &&
			voice->envs[0].state != ENV_OFF &&
			voice->envs[0].state != ENV_RELEASE) break;
	}
	if (!voice) return;
	for (int i = 0; i < NUM_ENVS; i++) {
		voice->envs[i].state = ENV_RELEASE;
	}
}


void synth_event(uint8_t type, uint8_t a, uint8_t b) {
	switch (type) {
	case 0x9:
		synth_note_on(a, b);
		break;

	case 0x8:
		synth_note_off(a);
		break;

	default: break;
	}
}


