// compile: gcc g++ -lm -lSDL -s -Os main.cpp

#include <stdio.h>
#include <math.h>
#include <SDL/SDL.h>

enum { MIXRATE = 44100 };

struct	CHANNEL {
	unsigned char	cutoff;
	unsigned char	reso;

	unsigned char	filter;
	unsigned char	volume;

	unsigned char	wave;
	unsigned char	detune;

//	for mixing
	unsigned char	state;

	float			speed[2];
	float			pos[2];

	float			volumeEnv;

	float			filterPos;
	float			filterDelta;
	float			filterEnv;
};

enum {
	SONGLENGTH		= 20,
	PATTERNLENGTH	= 16,
	NUMCHANNELS		= 7,
	TEMPO			= 5050,

	DELAYLENGTH		= TEMPO * 2,

	SAW		= 0x01,
	PULSE	= 0x02,
	SINE	= 0x03,
	NOISE	= 0x10,
	LFO		= 0x20,
};

float			delayBuffer[DELAYLENGTH];
unsigned int	delayPos;

CHANNEL	channels[NUMCHANNELS] = {
	//	cutoff	reso	filter	volume	wave			detune
	{	80,		140,	0x49,	0x1a,	PULSE,			20	},		//	bass
	{	0,		170,	0xb5,	0x79,	SAW | LFO,		5	},
	{	30,		200,	0x39,	0x16,	SAW,			80	},
	{	30,		200,	0x39,	0x16,	SAW,			80	},
	{	30,		200,	0x39,	0x16,	SAW,			80	},
	{	100,	100,	0x3f,	0x5c,	SINE | NOISE		},		//	snare
	{	255,	0,		0x1f,	0x1f,	SINE				},		//	base
};


char seq[NUMCHANNELS][SONGLENGTH] = {
	{  22, 0,  0,  0,	1,  1,  2,  3,		1,  1,  2,  3,		1,  1,  2,  3,		1,  1,  2,  3,	},	//	bass
	{  0,  0,  0,  0,	0,  0,  0,  0,		8,  8,  8,  9,		8,  8,  8,  9,		8,  8,  8,  9,	},	//	lead

	{ 10, 13, 16, 19,	10, 13, 16, 19,		10, 13, 16, 19,		10, 13, 16, 19,		10, 13, 16, 19,	},	//	pad1
	{ 11, 14, 17, 20,	11, 14, 17, 20,		11, 14, 17, 20,		11, 14, 17, 20,		11, 14, 17, 20,	},	//	pad2
	{ 12, 15, 18, 21,	12, 15, 18, 21,		12, 15, 18, 21,		12, 15, 18, 21,		12, 15, 18, 21,	},	//	pad3

	{  0,  0,  0,  0,	0,  0,  0,  0,		0,  0,  0,  0,		6,  6,  6,  7,		6,  6,  6,  7,	},	//	snare
	{  0,  0,  0,  0,	0,  0,  0,  0,		0,  0,  0,  0,		4,  4,  4,  5,		4,  4,  4,  5,	},	//	base

};


enum {
	C1=0,	Cs1=1,	D1=2,	Ds1=3,	E1=4,	F1=5,	Fs1=6,	G1=7,	Gs1=8,	A1=9,	As1=10,	B1=11,
	C2=12,	Cs2=13,	D2=14,	Ds2=15,	E2=16,	F2=17,	Fs2=18,	G2=19,	Gs2=20,	A2=21,	As2=22,	B2=23,
	C3=24,	Cs3=25,	D3=26,	Ds3=27,	E3=28,	F3=29,	Fs3=30,	G3=31,	Gs3=32,	A3=33,	As3=34,	B3=35,
	C4=36,	Cs4=37,	D4=38,	Ds4=39,	E4=40,	F4=41,	Fs4=42,	G4=43,	Gs4=44,	A4=45,	As4=46,	B4=47,
	C5=48,	Cs5=49,	D5=50,	Ds5=51,	E5=52,	F5=53,	Fs5=54,	G5=55,	Gs5=56,	A5=57,	As5=58,	B5=59,


	x=-1,	XX=-2
};


char patterns[][PATTERNLENGTH] = {
//	bass
	{	C2,	x,	C3,	C2,		XX,	XX,	C3,	x,		x,	x,	XX,	C2,		x,	x,	As1,x	},
	{	Gs1,x,	x,	Gs1,	XX,	XX,	Gs2,x,		x,	x,	x,	x,		x,	x,	Gs1,Gs2	},
	{	G1,	x,	G2,	G1,		XX,	x,	G2,	XX,		As1,x,	XX,	As1,	x,	XX,	G2,	As2	},

//	drum
	{	32,	XX,	x,	x,		x,	x,	x,	x,		32,	XX,	x,	x,		x,	x,	x,	x	},
	{	32,	XX,	x,	x,		x,	x,	x,	x,		32,	XX,	x,	x,		x,	x,	32,	XX	},
	{	x,	x,	x,	x,		42,	x,	x,	x,		x,	x,	x,	x,		42,	x,	x,	x	},
	{	x,	x,	x,	x,		42,	x,	x,	42,		x,	42,	x,	x,		42,	x,	x,	x	},

	{	C4,	D4,	Ds4,C4,		As4,Ds4,C4,	D4,		C4,	D4,	Ds4,C4,		C5,Ds4,C4,	D4	},
	{	C4,	D4,	Ds4,C4,		As4,Ds4,C4,	D4,		C4,	D4,	Ds4,C4,		C5,Ds4,C4,	As3	},

//	pad
	{	C4,	x,	x,	x,		C4,	x,	x,	As3,	x,	x,	C4,	x,		x,	x,	As3,x	},
	{	Ds4,x,	x,	x,		Ds4,x,	x,	D4,		x,	x,	Ds4,x,		x,	x,	D4,	x	},
	{	G4,	x,	x,	x,		G4,	x,	x,	F4,		x,	x,	G4,	x,		x,	x,	F4,	x	},

	{	C4,	x,	x,	x,		x,	x,	x,	As3,	x,	x,	C4,	x,		x,	x,	x,	x	},
	{	Ds4,x,	x,	x,		x,	x,	x,	D4,		x,	x,	Ds4,x,		x,	x,	x,	x	},
	{	G4,	x,	x,	x,		x,	x,	x,	F4,		x,	x,	G4,	x,		x,	x,	x,	x	},

	{	C4,	x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x	},
	{	Ds4,x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x	},
	{	As4,x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x,		x,	x,	Gs4,x	},

	{	Ds4,x,	x,	x,		Ds4,x,	x,	Ds4,	x,	x,	As3,x,		x,	x,	x,	x	},
	{	F4,	x,	x,	x,		F4,	x,	x,	F4,		x,	x,	D4,	x,		x,	x,	x,	x	},
	{	As4,x,	x,	x,		As4,x,	x,	As4,	x,	x,	F4,	x,		x,	x,	x,	x	},

//	stop bass
	{	XX,	x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x,		x,	x,	x,	x	},
};


unsigned int	curSeq = 4;
unsigned int	curRow;
unsigned int	curSample;


float lfo = 2;

float	Mix() {
	//	LFO
	lfo += 0.00001;
	if(lfo > M_PI * 2) lfo -= M_PI * 2;


	if(curSample == 0) {
		for(int c=0; c<NUMCHANNELS; c++) {
			int pat = seq[c][curSeq];
			if(pat == 0)
				continue;

			CHANNEL* chan = &channels[c];

			int note = patterns[--pat][curRow];
			if(note == -2) {
				chan->state = 0;
			}
			else if(note >= 0) {
				note -= 45;
				chan->state		= 1;
				chan->volumeEnv	= 1.0f;
				chan->filterEnv	= 1.0f;

				if((chan->wave & 0x0f) == SINE) {
					chan->pos[0] = 0.0f;
					chan->pos[1] = 1.0f;
					chan->speed[0] = pow(2, note / 12.0) * (440.0f / MIXRATE) * 2.0f * M_PI;
				}
				else {
					float detune = chan->detune * 0.0015f;
					chan->speed[0] = pow(2, (note + detune) / 12.0) * (440.0f / MIXRATE);
					chan->speed[1] = pow(2, (note - detune) / 12.0) * (440.0f / MIXRATE);
				}
			}

		}
	}


	float mix = 0.0f;

	for(int c=0; c<NUMCHANNELS; c++) {
		float amp = 0.0f;
		CHANNEL* chan = &channels[c];

		//	osc
		unsigned char wave = chan->wave & 0x0f;
		if(wave == SAW)
			amp = (chan->pos[0] + chan->pos[1])*1.0f - 1.0f;
		else if(wave == PULSE)
			amp = (chan->pos[0] > 0.5f ? -0.5f : 0.5f) + (chan->pos[1] > 0.5f ? -0.5f : 0.5f);
		else if(wave == SINE)
			amp = chan->pos[0] * 1.5f;

		if(chan->wave & NOISE)
			amp = amp*0.8f + (rand()/(float)RAND_MAX - 0.5f);

		if(wave == SINE) {
			chan->pos[0] += chan->pos[1] * chan->speed[0];
			chan->pos[1] -= chan->pos[0] * chan->speed[0];
		}
		else {
			for(int m=0; m<2; m++) {
				chan->pos[m] += chan->speed[m];
				chan->pos[m] -= (int)chan->pos[m];
			}
		}
		if(c >= NUMCHANNELS-2) {	//	pitch down
			chan->speed[0] *= 0.9995;
			chan->speed[1] *= 0.9995;
		}

		//	volume
		amp *= chan->volumeEnv * float(chan->volume & 0x0f) * (1.0f/15.0f);
		if(chan->state > 0)
			chan->volumeEnv *= 1.0f - float(chan->volume & 0xf0) * (0.00003f/15.0f);	//	decay
		else
			chan->volumeEnv *= 0.9985f;	//	release


//		filter
		float cutoff = float(chan->cutoff) * (1.0f/511.0f);
		cutoff += chan->filterEnv * float(chan->volume & 0x0f) * (0.8f/15.0f);
		if(chan->wave & LFO)
			cutoff += sin(lfo)*0.4f + 0.35f;

		chan->filterEnv *= 1.0f - float(chan->filter & 0xf0) * (0.00003f / 15.0f);	//	decay

		if(cutoff < 0.0f) cutoff = 0.0f;
		if(cutoff > 1.0f) cutoff = 1.0f;

		float d = cutoff * cutoff;

		chan->filterDelta	+= (amp - chan->filterPos) * d;
		chan->filterPos		+= chan->filterDelta;
		chan->filterDelta	*= float(chan->reso) * (1.0f / 255.0f);

		amp = chan->filterPos;

//		send-effects
		if(c == 1) {
			float t = delayBuffer[delayPos];
			delayBuffer[delayPos] *= 0.65f;	//	feedback
			delayBuffer[delayPos] += amp;

			amp += t * 0.4f;				//	delay-mix

			if(++delayPos == DELAYLENGTH)
				delayPos = 0;
		}

		mix += amp;
	}


	if(++curSample == TEMPO) {
		curSample = 0;
		if(++curRow == PATTERNLENGTH) {
			curRow = 0;
			if(++curSeq == SONGLENGTH)
				curSeq = 0;
		}
	}
	return mix;
}


void audio_callback(void* userdata, unsigned char* stream, int len) {
	short* buffer = (short*) stream;
	for (int i = 0; i < len >> 1; i++) {
		int m = Mix() * 8000;
		buffer[i] = m > 32767 ? 32767 : m < -32768 ? -32768 : m;
	}
}


int main(int argc, char** argv) {
	printf(	"Flat Synth Player v0.1\n\n"
			" Copyright (C) 2006 Daniel Langner\n"
			"___________________________________\n\n"
			" Press [ESC] to quit.\n\n"
			" Playing Song...\n");

	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		1, 0, 1024 / 2, 0, 0, &audio_callback, NULL
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);

	getchar();
}

