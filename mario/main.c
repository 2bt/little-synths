#include <stdio.h>
#include <math.h>

#include <SDL/SDL.h>

#define ARRAY_SIZE(array)		(sizeof(array)/sizeof(*array))


enum {
	MIXRATE = 44100,
	NOTE_LENGTH = 2500,

	end = -1,
	xxx = 0,

	g_0, gs0, a_0, as0, b_0,
	c_1, cs1, d_1, ds1, e_1, f_1, fs1, g_1, gs1, a_1, as1, b_1,
	c_2, cs2, d_2, ds2, e_2, f_2, fs2, g_2, gs2, a_2, as2, b_2,
	c_3, cs3, d_3, ds3, e_3, f_3, fs3, g_3, gs3, a_3, as3, b_3,
	c_4,
};


int patterns[][3][48] = {
//		#           .           .           .           #           .           .           .           #           .           .           .           #           .           .           .
	{
		{e_3,xxx,end,e_3,xxx,end,xxx,xxx,xxx,e_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,e_3,xxx,end,xxx,xxx,xxx,g_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{fs2,xxx,end,fs2,xxx,end,xxx,xxx,xxx,fs2,xxx,end,xxx,xxx,xxx,fs2,xxx,end,fs2,xxx,end,xxx,xxx,xxx,b_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{d_1,xxx,end,d_1,xxx,end,xxx,xxx,xxx,d_1,xxx,end,xxx,xxx,xxx,d_1,xxx,end,d_1,xxx,end,xxx,xxx,xxx,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx}
	},{
		{c_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,e_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,a_2,xxx,end,xxx,xxx,xxx,b_2,xxx,end,xxx,xxx,xxx,as2,xxx,end,a_2,xxx,end,xxx,xxx,xxx},
		{e_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,d_2,xxx,end,xxx,xxx,xxx,cs2,xxx,end,c_2,xxx,end,xxx,xxx,xxx},
		{g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,e_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,f_1,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,fs1,xxx,end,f_1,xxx,end,xxx,xxx,xxx}
	},{
		{g_2,xxx,xxx,end,e_3,xxx,xxx,end,g_3,xxx,xxx,end,a_3,xxx,end,xxx,xxx,xxx,f_3,xxx,end,g_3,xxx,end,xxx,xxx,xxx,e_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,d_3,xxx,end,b_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx},
		{c_2,xxx,xxx,end,g_2,xxx,xxx,end,b_2,xxx,xxx,end,c_3,xxx,end,xxx,xxx,xxx,a_2,xxx,end,b_2,xxx,end,xxx,xxx,xxx,a_2,xxx,end,xxx,xxx,xxx,e_2,xxx,end,f_2,xxx,end,d_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx},
		{e_1,xxx,xxx,end,c_2,xxx,xxx,end,e_2,xxx,xxx,end,f_2,xxx,end,xxx,xxx,xxx,d_2,xxx,end,e_2,xxx,end,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,a_1,xxx,end,b_1,xxx,end,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx}
	},{
		{xxx,xxx,xxx,xxx,xxx,xxx,g_3,xxx,end,fs3,xxx,end,f_3,xxx,end,ds3,xxx,end,xxx,xxx,xxx,e_3,xxx,end,xxx,xxx,xxx,gs2,xxx,end,a_2,xxx,end,c_3,xxx,end,xxx,xxx,xxx,a_2,xxx,end,c_3,xxx,end,d_3,xxx,end},
		{xxx,xxx,xxx,xxx,xxx,xxx,e_3,xxx,end,ds3,xxx,end,d_3,xxx,end,b_2,xxx,end,xxx,xxx,xxx,c_3,xxx,end,xxx,xxx,xxx,e_2,xxx,end,f_2,xxx,end,g_2,xxx,end,xxx,xxx,xxx,c_2,xxx,end,e_2,xxx,end,f_2,xxx,end},
		{c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,f_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_2,xxx,end,c_2,xxx,end,xxx,xxx,xxx,f_1,xxx,end,xxx,xxx,xxx}
	},{
		{xxx,xxx,xxx,xxx,xxx,xxx,g_3,xxx,end,fs3,xxx,end,f_3,xxx,end,ds3,xxx,end,xxx,xxx,xxx,e_3,xxx,end,xxx,xxx,xxx,c_4,xxx,end,xxx,xxx,xxx,c_4,xxx,end,c_4,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{xxx,xxx,xxx,xxx,xxx,xxx,e_3,xxx,end,ds3,xxx,end,d_3,xxx,end,b_2,xxx,end,xxx,xxx,xxx,c_3,xxx,end,xxx,xxx,xxx,g_3,xxx,end,xxx,xxx,xxx,g_3,xxx,end,g_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,e_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,c_2,xxx,end,xxx,xxx,xxx,f_3,xxx,end,xxx,xxx,xxx,f_3,xxx,end,f_3,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx}
	},{
		{xxx,xxx,xxx,xxx,xxx,xxx,ds3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,d_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{xxx,xxx,xxx,xxx,xxx,xxx,gs2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,f_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,e_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{c_1,xxx,end,xxx,xxx,xxx,gs1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,as1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,g_1,xxx,end,xxx,xxx,xxx,c_1,xxx,end,xxx,xxx,xxx}
	},{
		{c_3,xxx,end,c_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,d_3,xxx,end,xxx,xxx,xxx,e_3,xxx,end,c_3,xxx,end,xxx,xxx,xxx,a_2,xxx,end,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{gs2,xxx,end,gs2,xxx,end,xxx,xxx,xxx,gs2,xxx,end,xxx,xxx,xxx,gs2,xxx,end,as2,xxx,end,xxx,xxx,xxx,g_2,xxx,end,e_2,xxx,end,xxx,xxx,xxx,e_2,xxx,end,c_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{gs0,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,ds1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,gs1,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_0,xxx,end,xxx,xxx,xxx}
	},{
		{c_3,xxx,end,c_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,d_3,xxx,end,e_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{gs2,xxx,end,gs2,xxx,end,xxx,xxx,xxx,gs2,xxx,end,xxx,xxx,xxx,gs2,xxx,end,as2,xxx,end,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{gs0,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,ds1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,gs1,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_0,xxx,end,xxx,xxx,xxx}
	},{
		{e_3,xxx,end,c_3,xxx,end,xxx,xxx,xxx,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,gs2,xxx,end,xxx,xxx,xxx,a_2,xxx,end,f_3,xxx,end,xxx,xxx,xxx,f_3,xxx,end,a_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{c_3,xxx,end,a_2,xxx,end,xxx,xxx,xxx,e_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,e_2,xxx,end,xxx,xxx,xxx,f_2,xxx,end,c_3,xxx,end,xxx,xxx,xxx,c_3,xxx,end,f_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,fs1,xxx,end,g_1,xxx,end,xxx,xxx,xxx,c_2,xxx,end,xxx,xxx,xxx,f_1,xxx,end,xxx,xxx,xxx,f_1,xxx,end,xxx,xxx,xxx,c_2,xxx,end,c_2,xxx,end,f_1,xxx,end,xxx,xxx,xxx}
	},{
		{b_2,xxx,xxx,end,a_3,xxx,xxx,end,a_3,xxx,xxx,end,a_3,xxx,xxx,end,g_3,xxx,xxx,end,f_3,xxx,xxx,end,e_3,xxx,end,c_3,xxx,end,xxx,xxx,xxx,a_2,xxx,end,g_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{g_2,xxx,xxx,end,f_3,xxx,xxx,end,f_3,xxx,xxx,end,f_3,xxx,xxx,end,e_3,xxx,xxx,end,d_3,xxx,xxx,end,c_3,xxx,end,a_2,xxx,end,xxx,xxx,xxx,f_2,xxx,end,e_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{d_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,f_1,xxx,end,g_1,xxx,end,xxx,xxx,xxx,b_1,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,c_2,xxx,end,c_2,xxx,end,g_1,xxx,end,xxx,xxx,xxx}
	},{
		{b_2,xxx,end,f_3,xxx,end,xxx,xxx,xxx,f_3,xxx,end,f_3,xxx,xxx,end,e_3,xxx,xxx,end,d_3,xxx,xxx,end,c_3,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{g_2,xxx,end,d_3,xxx,end,xxx,xxx,xxx,d_3,xxx,end,d_3,xxx,xxx,end,c_3,xxx,xxx,end,b_2,xxx,xxx,end,g_2,xxx,end,e_2,xxx,end,xxx,xxx,xxx,e_2,xxx,end,c_2,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx},
		{g_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,g_1,xxx,end,g_1,xxx,xxx,end,a_1,xxx,xxx,end,b_1,xxx,xxx,end,g_2,xxx,end,xxx,xxx,xxx,g_1,xxx,end,xxx,xxx,xxx,c_1,xxx,end,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx,xxx}
	}
};


int order[] = {
	0,
	1, 2, 1, 2,
	3, 4, 3, 5,
	3, 4, 3, 5,
	6, 7, 6, 0,
	1, 2, 1, 2,
	8, 9, 8, 10,
	8, 9, 8, 10,
	6, 7, 6, 0,
	8, 9, 8, 10
};



float osc[3];
float speed[3];
int sample = 0;
int note = 0;
int row = 0;




float mix() {
	if (sample == 0) {
		for (int i = 0; i < 3; i++) {
			int n = patterns[order[row]][i][note];
			if (n == -1) {
				osc[i] = 0;
				speed[i] = 0;
			}
			else if (n > 0) speed[i] = pow(2, n / 12.0) * 0.002;

		}
	}
	if (++sample >= NOTE_LENGTH) {
		sample = 0;
		if (++note >= 48) {
			note = 0;
			if (++row >= ARRAY_SIZE(order)) row = 1;
		}
	}

	osc[0] += speed[0];
	osc[1] += speed[1];
	osc[2] += speed[2];

	osc[0] -= (int)osc[0];
	osc[1] -= (int)osc[1];
	osc[2] -= (int)osc[2];

	return ((osc[0] > 0.1) +
			(osc[1] > 0.1) +
			(osc[2] > 0.5)) * 2 - 1.5;
}

void audio_callback(void* userdata, unsigned char* stream, int len) {

	short* buffer = (short*) stream;
	for (int i = 0; i < len / 2; i++) *buffer++ = mix() * 3000;
}


int main(int argc, char **argv) {

	static SDL_AudioSpec spec = { MIXRATE, AUDIO_S16SYS,
		1, 0, 1024, 0, 0, &audio_callback, NULL
	};
	SDL_OpenAudio(&spec, &spec);
	SDL_PauseAudio(0);
	puts("playing...");
	getchar();
	puts("exiting...");
	SDL_Quit();

	return 0;
}

