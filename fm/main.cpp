#include <cstdlib>
#include <cmath>
#include <SDL/SDL.h>
#include <vector>
#include "keyboard.h"

using namespace std;


enum { MIXRATE = 44100 };

int note = -12;
double pitch;


struct Operator {
	struct Settings {
		double attack;
		double decay;
		double release;
		double volume;
		double tuning;
	};

	Settings& settings;
	double phase;
	double env;
	int state;

	void tick(double pitch) {
		phase += pitch * settings.tuning;
		phase -= (int) phase;

		switch (state) {
		case 0:
			env *= settings.release;
			break;

		case 1:
			env += settings.attack;
			if (env >= 1) state = 2;
			break;
		case 2:
			env *= settings.decay;
			break;

		}
	}

	double amp(double shift=0) {
		return sin((phase + shift) * 2 * M_PI) * env * settings.volume;
	}

};



Operator::Settings opSet1 = { 0.1,    0.999997, 0.99992, 1,   1 };
Operator::Settings opSet2 = { 0.01,  0.99999,  0.9999, 0.3,   3 };
Operator::Settings opSet3 = { 0.0001, 0.9999,  0.999, 1,      5 };


struct Voice {
	int note;
	double pitch;

	double buffer[2];

	Operator op1 = {opSet1};
	Operator op2 = {opSet2};
	Operator op3 = {opSet2};

	void tick() {
		op1.tick(pitch);
		op2.tick(pitch);
		op3.tick(pitch);

		buffer[0] = op1.amp(op2.amp(op3.amp(0.00) + 0.00) + 0.00);
		buffer[1] = op1.amp(op2.amp(op3.amp(0.25) + 0.25) + 0.25);
	}

	void play(int n) {
		note = n;
		pitch = 440.0 / MIXRATE * pow(2, note / 12.0);
		op1.state = op2.state = op3.state = 1;
		op1.env = op2.env = op3.env = 0;
	}
	void release() {
		op1.state = op2.state = op3.state = 0;
	}
	double getBusiness() {
		return	op1.env + (op1.state != 0) +
				op2.env + (op2.state != 0) +
				op3.env + (op3.state != 0);
	}

};


vector<Voice> voices(8);

void playNote(int note) {
	Voice* best = &voices[0];
	double busy = best->getBusiness();
	for (Voice& v : voices) {
		double b = v.getBusiness();
		if (b < busy) {
			busy = b;
			best = &v;
		}
	}
	best->play(note);
}

void releaseNote(int note) {
	for (Voice& v : voices) {
		if (v.note == note) v.release();
	}
}


void audio_callback(void* userdata, unsigned char* stream, int len) {


	KeyboardEvent e;
	while(keyboard_poll(&e)) {
		if(e.x >= 95) continue;
		if(e.type == 0x90) playNote(e.x - 62);
		else if(e.type == 0x80) releaseNote(e.x - 62);
	}



	short* buffer = (short*) stream;
	for (int i = 0; i < len / 4; i++, buffer += 2) {


		double b[2] = {0, 0};
		for (Voice& v : voices) {
			v.tick();
			b[0] += v.buffer[0];
			b[1] += v.buffer[1];
		}

		buffer[0] = b[0] * 6000;
		buffer[1] = b[1] * 6000;
	}

}


int main(int argc, char **argv) {

	keyboard_init();
	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		2, 0, 1024, 0, 0, &audio_callback, NULL
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
	puts("playing...");
	getchar();
	puts("exiting...");
	SDL_Quit();
	keyboard_kill();

	return 0;
}

