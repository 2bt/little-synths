//	little_synth.c

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>
#include <pulse/simple.h>


enum {
	MIXRATE			= 44100,
	TEMPO			= 4000,
	channel_count	= 2,
	song_length		= 16 * 8
};

enum { RELEASE, ATTACK, HOLD };
typedef struct {
	float	phase;
	float	speed;
	float	level;
	int		state;
	int		note;
} CHANNEL;

enum {
	con = 0,
	end = -1,
	c_0 = 0, cs0 = 1, d_0 = 2, ds0 = 3, e_0 = 4, f_0 = 5, fs0 = 6, g_0 = 7, gs0 = 8, a_0 = 9, as0 = 10, b_0 = 11,
	c_1 = 12, cs1 = 13, d_1 = 14, ds1 = 15, e_1 = 16, f_1 = 17, fs1 = 18, g_1 = 19, gs1 = 20, a_1 = 21, as1 = 22, b_1 = 23,
	c_2 = 24, cs2 = 25, d_2 = 26, ds2 = 27, e_2 = 28, f_2 = 29, fs2 = 30, g_2 = 31, gs2 = 32, a_2 = 33, as2 = 34, b_2 = 35,
	c_3 = 36, cs3 = 37, d_3 = 38, ds3 = 39, e_3 = 40, f_3 = 41, fs3 = 42, g_3 = 43, gs3 = 44, a_3 = 45, as3 = 46, b_3 = 47,
};

const char music_data[2][song_length] =  {	{
		e_1, con, e_2, con, e_1, con, e_2, con, e_1, con, e_2, con, e_1, con, e_2, con,
		a_0, con, a_1, con, a_0, con, a_1, con, a_0, con, a_1, con, a_0, con, a_1, con,
		gs0, con, gs1, con, gs0, con, gs1, con, gs0, con, gs1, con, gs0, con, gs1, con,
		a_0, con, a_1, con, a_0, con, a_1, con, a_0, con, a_0, con, b_0, con, c_1, con,

		d_1, con, d_2, con, d_1, con, d_2, con, d_1, con, d_2, con, d_1, con, d_2, con,
		c_1, con, c_2, con, c_1, con, c_2, con, c_1, con, c_2, con, c_1, con, c_2, con,
		b_0, con, b_1, con, b_0, con, b_1, con, b_0, con, b_1, con, b_0, con, b_1, con,
		a_0, con, a_1, con, a_0, con, a_1, con, a_0, con, con, con, end, con, con, con,
	}, {
		e_3, con, con, con, b_2, con, c_3, con, d_3, con, e_3, d_3, c_3, con, b_2, con,
		a_2, con, con, con, a_2, con, c_3, con, e_3, con, con, con, d_3, con, c_3, con,
		b_2, con, con, con, con, con, c_3, con, d_3, con, con, con, e_3, con, con, con,
		c_3, con, con, con, a_2, con, con, con, a_2, con, con, con, con, con, con, con,

		end, con, d_3, con, con, con, f_3, con, a_3, con, con, con, g_3, con, f_3, con,
		e_3, con, con, con, con, con, c_3, con, e_3, con, con, con, d_3, con, c_3, con,
		b_2, con, con, con, b_2, con, c_3, con, d_3, con, con, con, e_3, con, con, con,
		c_3, con, con, con, a_2, con, con, con, a_2, con, con, con, end, con, con, con,
}	};

CHANNEL	channel_list[channel_count];
int		cur_sample	= 0;
int		cur_row		= 0;

inline float	Mix() {
	int c;
	if(cur_sample == 0) {
		for(c = 0; c < channel_count; c++) {
			CHANNEL *chan = &channel_list[c];

			int n = chan->note;
			chan->note = music_data[c][cur_row];
			if(n == end) chan->state = RELEASE;
			else if(n != con) {
				chan->speed = (440.0 / (float)MIXRATE) *
					pow(2, (float)(n - 9 - (12 << 1)) * (1.0 / 12.0));
				chan->state = ATTACK;
			}
		}
	}
	else if(cur_sample == TEMPO * 3 / 4) {
		for(c = 0; c < channel_count; c++) {
			if(channel_list[c].note != con)
				channel_list[c].state = RELEASE;
		}
	}
	cur_sample++;
	if(cur_sample == TEMPO) {
		cur_sample = 0;
		cur_row = (cur_row + 1) % song_length;
	}
	float out = 0;
	for(c = 0; c < channel_count; c++) {
		CHANNEL *chan = &channel_list[c];
		chan->phase += chan->speed;
		chan->phase -= (int) chan->phase;
		switch(chan->state) {
			case	ATTACK:
				chan->level += 0.005;
				if(chan->level >= 1) chan->state = HOLD;
				break;
			case	HOLD:
				chan->level = 0.5 + (chan->level - 0.5) * 0.9999;
				break;
			default:
				chan->level *= 0.998;
		}
		float amp;
		if(c == 1) amp = (chan->phase < 0.5) ? -1 : 1;
		else amp = 1.3 - chan->phase * 2.6;
		out += amp * chan->level;
	}
	// some crude low-pass filtering
	static float f = 0;
	out = f + (out - f) * 0.3;
	f = out;

	return out;
}

pa_simple* pa;
int running = 1;

static void leave(int sig) {
	running = 0;
}

int main(int argc, char **argv) {
	static const pa_sample_spec pass = { PA_SAMPLE_S16LE, MIXRATE, 1 };
	static short buffer[1024];
	int m;
	pa = pa_simple_new(NULL, argv[0], PA_STREAM_PLAYBACK, NULL, argv[0], &pass, NULL, NULL, NULL);
	if(!pa) {
		puts("error");
		return 1;
	}
	signal(SIGINT, leave);
	while(running) {
		for(m = 0; m < 1024; m++) buffer[m] = Mix() * 5000;
		pa_simple_write(pa, buffer, sizeof(buffer), NULL);
	}
	pa_simple_free(pa);
	return 0;
}

