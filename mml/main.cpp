#if 0
g++ -Wall -std=c++11 -O3 -xc++ $0 -lSDL2 && ./a.out
#rm -f a.out
exit
#endif

#include <cstdlib>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <array>
#include <SDL2/SDL.h>

/*
std::vector<float> envelopes[] = {
	{ 0, 0 },
	{ -2, -1.66, -1.33, -1, -0.66, -0.33, 0, 6 },
	{ -2, -2, -2, -2, 0, 0, 0, 0, -2, 8 },
	{ 0, 0, 3, 3, 7, 7, 10, 10, 0},
	{ 0, 0, 4, 4, 7, 7, 12, 12, 0},
};
void init_voices() {
	for (Voice& v : voices) v.state = OFF;
	instruments[ 0 ] = { 1,	PULSE, 0.5, 0,	{ 0.01,  0.5, 0.9999,	0.9992 }  };
	instruments['l'] = { 0.8, PULSE, 0.2, 0.2, { 0.01,  0.5, 0.9999,	0.9992 }, 0.15, 0.09, false, 0, 0, 0.3 };
	instruments['p'] = { 0.3, PULSE, 0.3,-0.3, { 0.001, 0.0, 0.999992, 0.9999 }, 0.3,  0.05 };
	instruments['b'] = { 1.8, PULSE, 0.3, 0.1, { 0.01,  0.5, 0.9998,	0.9992 }, 0, 0.2, true, 0.9, -4.5 };
	instruments['m'] = { 1.2, SAW,	0.5,-0.1, { 0.0005,0.5, 0.99993,  0.99993}, 0.35, 0.09, false, 0, 0, 0.1 };
}

# envelopes
0@ -2 -1 0
1@ |0 3 7

# instrument
0: 1 @2 3 4 4646

I0 c '0=1.3 '9@1

*/

const char* src = R"(

@8=|0 7 10 17
@9=|0 7 10 15

:1=w0 o@0

#a8'o@8 a8'o@9

L2Q4O2	a'e0.5'p.9/>> a'o@0 r16<a'v.1>b'w1'p-.2<a'v.1>c'u.5'e0.2'p.3 e'v.2
O1		a16'w2'r0.99993



#IlQ6L2c<c>d<c>d+<c1c1>d<c>c<c>f<c>d+<c>d<c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<cccccccc
#
#IlQ6L2c<c>d<c>d+<c1c1>f<c>d+<c>d<c>c<ca+c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<cccccccc
#
#IlQ6L2c<c>d<c>d+<c1c1>d<c>c<c>f<c>d+<c>d<c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<<g+g+g+g+g+g+g+g+
#
#IlQ6L2c<c>d<c>d+<c1c1>f<c>d+<c>d<c>c<ca+c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<<g+g+g+g+a+a+a+a+
#
#IlQ6L2c<c>d<c>d+<c1c1>d<c>c<c>f<c>d+<c>d<c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<cccccccc
#Q7Ima+30f2
#
#IlQ6L2c<c>d<c>d+<c1c1>f<c>d+<c>d<c>c<ca+c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<cccccccc
#Q7Img26g2a+2g2
#
#IlQ6L2c<c>d<c>d+<c1c1>d<c>c<c>f<c>d+<c>d<c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<<g+g+g+g+g+g+g+g+
#Q7Im>d+3d3<a+2>c18c2<g2f34/r2
#
#IlQ6L2c<c>d<c>d+<c1c1>f<c>d+<c>d<c>c<ca+c1c1
#IpA3>c20r4<A4a+8
#Q7Ib<<<g+g+g+g+a+a+a+a+




# Laxity - Basic


#IbQ4O2L2 cc4cr6c <a+4a+4r>a+4<a+ aa4ar6a g+4g+4r>g+4<g+ gg4gr6g >d4d4r>d4<d cc4cr6c <f4f4r>g4<g
#IpL16Q7<g/>c/e<g+/>c/f<g/>c/e <g+/>c/f< a+/>c/f<a/>c/f<g/>c/eL8<f/a/>c<g/b/>d


#IlL2Q7>e12def4ec4d4 e14deA1f4Aec4d4 d17A2d3A<g>cdA1eAdc<g> e16f8g8
#IbQ4O2L2 cc4cr6c <a+4a+4r>a+4<a+ aa4ar6a g+4g+4r>g+4<g+ gg4gr6g >d4d4r>d4<d cc4cr6c <f4f4r>g4<g
#IpL16Q7<g/>c/e<g+/>c/f<g/>c/e <g+/>c/f< a+/>c/f<a/>c/f<g/>c/eL8<f/a/>c<g/b/>d
#
#IlL2Q7>g10>A1dA<b>c6<b3r1b4a3r1A1a3Ag3ad+1d1c6r8Q8d+8Q7e16g+4a4b8b1a1Q8g8Q7arg4a
#IbQ4O2L2 ee4er6e <a4a4r>a4<a >dd4dr6d <a+4a+4r>a+4<a+ aa4ar6a g+4g+4r>g+4<g+ gg4gr6g g4g4r>g4<g
#IpL16Q7<b/>d/g<g/>c/e<a/>c/f<g+/>c/f<g/>c/e<f+/>c/e<f/>c/e<g/b/>d

)";



enum {
	MIXRATE			= 48000,
	VOICE_COUNT		= 32,
	TRACK_COUNT		= 32,
	SAMPLES_PER_ROW = 5500, // TODO: remove this
};


struct Env {
	std::vector<float>	data;
	int					loop;
};


struct Param {
public:
	Param& operator=(const Env& e) {
		m_env = &e;
		m_pos = 0;
		return *this;
	}
	Param& operator=(float v) {
		m_val = v;
		m_env = nullptr;
		return *this;
	}
	operator float() const { return m_val; }
	void tick() {
		if (!m_env || m_env->data.empty()) return;
		m_val = m_env->data[m_pos];
		if (++m_pos >= (int) m_env->data.size()) m_pos = m_env->loop;
	}
private:
	const Env*	m_env;
	int			m_pos;
	float		m_val;
};


static const char* inst_lut = "vwoup" "asdr" "it" "frc" "e";
union Inst {
	std::array<Param, 15> params;
	struct {
		Param vol;
		Param wave;
		Param offset;
		Param pw;
		Param pan;

		Param attack;
		Param sustain;
		Param decay;
		Param release;

		Param vib;
		Param sweep;

		Param filter;
		Param reso;
		Param cutoff;

		Param echo;
	};
};



struct Track {
	enum { LEN, OCT, QUANT, INST };
	const char*			pos = nullptr;
	const char*			loop_pos;
	int					loop_count;
	std::array<int, 4>	state;
	int					wait;
};


class Echo {
public:
	void add(float l, float r) {
		buffer[m_pos][1] += l;
		buffer[m_pos][0] += r;
	}
	void add_mix(float* out) {
		m_pos = (m_pos + 1) % buffer.size();
		out[0] += buffer[m_pos][0];
		out[1] += buffer[m_pos][1];
		std::swap(buffer[m_pos][0], buffer[m_pos][1]);
		buffer[m_pos][0] *= m_feedback;
		buffer[m_pos][1] *= m_feedback;
	}
private:
	std::array<float[2], SAMPLES_PER_ROW * 3> buffer;
	int	m_pos = 0;
	float m_feedback = 0.4;
};


struct Voice {
	enum State { OFF, HOLD, ATTACK };
	State	state = OFF;
	Inst	inst;
	float	pitch;
	int		length;
	int		sample;
	float	level;
	float	pos;
	float	high;
	float	band;
	float	low;
};


class Tune {
public:

	Tune(const char* src) {
		m_src = m_pos = src;
		skip();
		m_src = m_pos;



		// default envelope
		m_envs.emplace_back();
		m_envs.back().data = { 5, 0 };
		m_envs.back().loop = 1;

		// default instrument
		m_insts.emplace_back();
		Inst& i = m_insts.back();
		i.vol		= 1;
		i.attack	= 0.005;
		i.sustain	= 0.5;
		i.decay		= 0.9999;
		i.release	= 0.999;
		i.pw		= 0.25;
		i.offset	= 0;
	}


	void add_mix(float* out) {

		if (m_sample == 0) tick();
		if (++m_sample > m_samples_per_row) m_sample = 0;

		for (Voice& v : m_voices) {

			if (v.sample % 1375 == 0) {
				for (Param& p : v.inst.params) p.tick();
				// TODO: cache parameter transformations
			}
			if (++v.sample > v.length) v.state = Voice::OFF;

			switch (v.state) {
			case Voice::ATTACK:
				v.level += v.inst.attack;
				if (v.level > 1) v.state = Voice::HOLD;
				break;
			case Voice::HOLD:
				v.level = v.inst.sustain + (v.level - v.inst.sustain) * v.inst.decay;
				break;
			default:
				v.level *=  v.inst.release;
				if (v.level <= 0.01) continue;
				break;
			}
			float pitch = v.pitch + v.inst.offset + sinf(v.sample * 0.0008) * v.inst.vib * (v.sample > 20000);
			float speed = exp2f((pitch - 57) / 12) * 440 / MIXRATE;
			v.pos = fmod(v.pos + speed, 1);
//			v.pw = fmod(v.pw + v.inst.sweep * 0.0001, 1);
			float amp = 0;
			switch ((int) v.inst.wave) {
			case 0: amp = v.pos > v.inst.pw ? -1 : 1; break;
			case 1: amp = v.pos * 2 - 1; break;
			case 2: amp = v.pos < 0.5 ? v.pos * 4 - 1 : 3 - v.pos * 4; break;
			default: break;
			}
			if (v.inst.filter) {
				float f = exp2f(v.inst.cutoff);
				v.low += f * v.band;
				v.high = amp - v.band * v.inst.reso - v.low;
				v.band += f * v.high;
				amp = v.low;
			}
			amp *= v.level * v.inst.vol;
			float buf[2] = { amp * sqrtf(0.5 - v.inst.pan * 0.5), amp * sqrtf(0.5 + v.inst.pan * 0.5), };
			out[0] += buf[0];
			out[1] += buf[1];
			if (v.inst.echo > 0) m_echo.add(buf[0] * v.inst.echo, buf[1] * v.inst.echo);
		}
		m_echo.add_mix(out);
	}

private:

	void tick(Track& t) {
		if (!t.pos) return;
		for (--t.wait; t.wait <= 0;) {
			while (memchr(" \t", *t.pos, 2)) ++t.pos;
			if (*t.pos == '[') {
				t.loop_pos = ++t.pos;
				t.loop_count = 0;
				continue;
			}
			if (*t.pos == ']') {
				assert(t.loop_pos);
				++t.pos;
				if (++t.loop_count < (int) strtoul(t.pos, (char**) &t.pos, 10)) t.pos = t.loop_pos;
				else t.loop_pos = nullptr;
				continue;
			}
			if (memchr("><", *t.pos, 2)) {
				t.state[Track::OCT] += *t.pos++ == '>' ? 1 : -1;
				continue;
			}
			static const char* state_lut = "LOQI";
			if (const char* p = (const char*) memchr(state_lut, *t.pos, 4)) {
				++t.pos;
				t.state[p - state_lut] = strtoul(t.pos, (char**) &t.pos, 10);
				continue;
			}
			if (*t.pos == 'r') {
				++t.pos;
				if (isdigit(*t.pos)) t.wait = strtoul(t.pos, (char**) &t.pos, 10);
				else t.wait = t.state[Track::LEN];
				continue;
			}
			static const char* note_lut = "ccddeffggaab";
			if (const char* p = (const char*) memchr(note_lut, *t.pos, 12)) {
				++t.pos;
				int note = (p - note_lut) + t.state[Track::OCT] * 12;
				while (memchr("-+", *t.pos, 2)) note += *t.pos++ == '+' ? 1 : -1;
				t.wait = strtoul(t.pos, (char**) &t.pos, 10) ?: t.state[Track::LEN];

				Voice* best = nullptr;
				for (Voice& w : m_voices) {
					if (!best || w.state < best->state
					|| (w.state == best->state && w.level < best->level)) best = &w;
				}
				Voice& v = *best;

				// TODO: bounds check
				if (t.state[Track::INST] < (int) m_insts.size()) {
					v.inst = m_insts[t.state[Track::INST]];
				}
				v.state		= Voice::ATTACK;
				v.pitch		= note;
				v.length	= t.wait * m_samples_per_row * t.state[Track::QUANT] / 8;
				v.sample	= 0;
				v.level		= 0;
				v.pos		= 0;
				v.high		= 0;
				v.band		= 0;
				v.low		= 0;

				// parse instrument settings
				while (*t.pos == '\'') {
					++t.pos;
					if (const char* p = (const char*) memchr(inst_lut, *t.pos, strlen(inst_lut))) {
						++t.pos;
						if (*t.pos == '@') {
							++t.pos;
							int env = strtoul(t.pos, (char**) &t.pos, 10);
							if (env < (int) m_envs.size()) v.inst.params[p - inst_lut] = m_envs[env];
						}
						else {
							v.inst.params[p - inst_lut] = strtof(t.pos, (char**) &t.pos);
						}
					}
				}

				// polyphony
				if (*t.pos == '/') {
					++t.pos;
					t.wait = 0;
				}
				continue;
			}
			// can't parse any further
			t.pos = nullptr;
			break;
		}
	}

	void parse_env() {
		++m_pos;
		int i = strtoul(m_pos, (char**) &m_pos, 10);
		if ((int) m_envs.size() < i + 1) m_envs.resize(i + 1);
		Env& e = m_envs[i];
		if (!e.data.empty()) return;
		e.loop = -1;
		if (*m_pos != '=') return;
		++m_pos;
		for(;;) {
			while (memchr(" \t", *m_pos, 2)) ++m_pos;
			const char* p = m_pos;
			float f = strtof(m_pos, (char**) &m_pos);
			if (m_pos != p) {
				e.data.push_back(f);
			}
			else if (*m_pos == '|') {
				++m_pos;
				e.loop = e.data.size();
			}
			else break;
		}
		if (e.loop == -1) e.loop = e.data.size();
	}

	void parse_inst() {
		++m_pos;
		int i = strtoul(m_pos, (char**) &m_pos, 10);
		if ((int) m_insts.size() < i + 1) m_insts.resize(i + 1);
		Inst& inst = m_insts[i];
	}

	void skip(bool lines=true) {
		for (;;) {
			while (memchr("\n\t  " + !lines, *m_pos, 3)) ++m_pos;
			if (*m_pos != '#') break;
			while (*m_pos && *m_pos != '\n') ++m_pos;
			m_pos += (*m_pos == '\n');
		}
	}

	void tick() {
		for (;;) {
			bool idle = true;
			for (Track& t : m_tracks) {
				tick(t);
				idle &= (t.pos == nullptr);
			}
			if (!idle) break;

			//if (!m_pos) break;

			for (int i = 0; i < (int) m_tracks.size() && *m_pos && *m_pos != '\n'; ++i) {

				if (*m_pos == '@') parse_env();
				else if (*m_pos == ':') parse_inst();
				else {
					Track& t		= m_tracks[i];
					t.pos			= m_pos;
					t.loop_pos		= nullptr;
					t.loop_count	= 0;
					t.state			= { 4, 4, 7, 0 };
					t.wait			= 0;
				}

				// next line
				while (*m_pos && *m_pos != '\n') ++m_pos;
				m_pos += (*m_pos == '\n');
				skip(false);
			}

			skip();
			if (!*m_pos) {
				m_pos = m_src;
			}

		}
	}

	const char*							m_src;
	const char*							m_pos;
	std::array<Track, TRACK_COUNT>		m_tracks;

	std::vector<Env>					m_envs;
	std::vector<Inst>					m_insts;

	int									m_samples_per_row = 5500;
	int									m_sample = 0;
	std::array<Voice, VOICE_COUNT>		m_voices;
	Echo								m_echo;
};


Tune tune(src);

void cb(void* u, Uint8* stream, int len) {
	short* buf = (short*) stream;
	for (; len > 0; len -= 4) {
		float out[2] = {};
		tune.add_mix(out);
		*buf++ = std::max(-32768, std::min<int>(out[0] * 6000, 32767));
		*buf++ = std::max(-32768, std::min<int>(out[1] * 6000, 32767));
	}
}

int main() {
	SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0, &cb };
	SDL_OpenAudio(&spec, nullptr);
	SDL_PauseAudio(0);
	getchar();
	SDL_Quit();
	return 0;
}
