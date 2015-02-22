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
		Osc* osc = &voice->oscs[i];
		OscPatch* osc_patch = &voice->patch.oscs[i];

		if (osc_patch->mode == OSC_OFF) continue;

		float pitch = osc_patch->transpose * 12 +
					  osc_patch->detune +
					  voice->note - 57;

		osc->phase += 440.0 * exp2f(pitch / 12.0) / MIXRATE;
		osc->phase -= (int) osc->phase;

		float f;
		switch (osc_patch->mode) {
		case OSC_PULSE:
		default:
			f = osc->phase < osc_patch->pulsewidth ? -1 : 1;
			break;
		case OSC_TRIANGLE:
			f = osc->phase < osc_patch->pulsewidth  ?
				2 / osc_patch->pulsewidth * osc->phase - 1 :
				2 / (osc_patch->pulsewidth - 1) * (osc->phase - osc_patch->pulsewidth) + 1;
			break;
		case OSC_SINE:
			f = sinf(osc->phase * 2 * M_PI);
			break;
		}
		buf += f * osc_patch->volume;
	}

	// envs
	for (int i = 0; i < NUM_ENVS; i++) {
		Env* env = &voice->envs[i];
		EnvPatch* env_patch = &voice->patch.envs[i];

		float a = expf(env_patch->attack * -9 - 5);
		float d = 1 - expf(env_patch->decay * -4 - 7);
		float r = 1 - expf(env_patch->release * -4 - 7);

		switch (env->state) {
		case ENV_OFF: continue;
		case ENV_ATTACK:
			env->level += a;
			if (env->level > 1) {
				env->level = 1;
				env->state = ENV_HOLD;
			}
			break;
		case ENV_HOLD:
			env->level = env_patch->sustain + (env->level - env_patch->sustain) * d;
			break;
		case ENV_RELEASE:
		default:
			env->level *= r;
			if (env->level < 0.0005) {
				env->level = 0;
				env->state = ENV_OFF;
			}
			break;
		}
	}

	// lfos
	for (int i = 0; i < NUM_LFOS; i++) {
		LFOsc* lfo = &voice->lfos[i];
		LFOscPatch* lfo_patch = &voice->patch.lfos[i];

		float speed = exp2f(lfo_patch->rate * 6 - 2) / MIXRATE;
		lfo->phase += speed;
		lfo->phase -= (int) lfo->phase;

		float buf = sinf(lfo->phase * M_PI * 2);
		lfo->out = buf * lfo_patch->amplify;
	}


	buf *= expf(voice->velocity * 2 - 2);
	buf *= voice->envs[0].level;

	out[0] += buf * sqrtf(0.5 - voice->patch.panning * 0.5);
	out[1] += buf * sqrtf(0.5 + voice->patch.panning * 0.5);
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

		for (int p = 0; p < NUM_CORDS; p++) {
			PatchCord* cord = &synth.patch->cords[p];
			if (!cord->enabled) continue;
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


