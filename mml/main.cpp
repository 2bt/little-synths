#if 0
g++ -Wall -std=c++11 -O3 -xc++ $0 -lSDL2 && ./a.out
rm -f a.out
exit
#endif

#include <cstdlib>
#include <assert.h>
#include <algorithm>
#include <vector>
#include <array>
#include <SDL2/SDL.h>

const char* tune = R"(

IlL2Q7>e12def4ec4d4 e14deA1f4Aec4d4 d17A2d3A<g>cdA1eAdc<g> e16f8g8
IbQ4O2L2 cc4cr6c <a+4a+4r>a+4<a+ aa4ar6a g+4g+4r>g+4<g+ gg4gr6g >d4d4r>d4<d cc4cr6c <f4f4r>g4<g
IpL16Q7<g/>c/e<g+/>c/f<g/>c/e<g+/>c/f<a+/>d/f<a/>c/f<g/>c/eL8<f/a/>c<g/b/>d

IlL2Q7>e12def4ec4d4 e14deA1f4Aec4d4 d17A2d3A<g>cdA1eAdc<g> e16f8g8
IbQ4O2L2 cc4cr6c <a+4a+4r>a+4<a+ aa4ar6a g+4g+4r>g+4<g+ gg4gr6g >d4d4r>d4<d cc4cr6c <f4f4r>g4<g
IpL16Q7<g/>c/e<g+/>c/f<g/>c/e<g+/>c/f<a+/>d/f<a/>c/f<g/>c/eL8<f/a/>c<g/b/>d

IlL2Q7>g10>A1dA<b>c6<b3r1b4a3r1A1a3Ag3ad+1d1c6r8d+8e16g+4a4b8b1a1g8arg4a
IbL2Q5O2ee>e<ee>e<e>e<<aa>a<aa>a<a>add>d<dd>d<d>d<<a+a+>a+<a+a+>a+<a+>a+<aa>a<aa>a<a>a<g+g+>g+<g+g+>g+<g+>g+<gg>g<gg>g<g>g<gg>g<gg>g<g>g
IpL16Q7<b/>d/g<g/>c/e<a/>c/f<g+/>c/f<g/>c/e<f+/>c/e<f/>c/e<g/b/>d

)";

enum {
   MIXRATE         = 48000,
   POLY            = 32,
   SAMPLES_PER_ROW = 5500,
};
enum State { OFF, HOLD, ATTACK };
enum Wave { PULSE, SAW, TRI };
struct Instrument {
   float vol;
   Wave  wave;
   float pw;
   float pan;
   float env[4];
   float vib;
   float sweep;
   bool  filter;
   float reso;
   float cutoff;
};
struct Voice : Instrument {
   int   sample = 0;
   State state  = ATTACK;
   float level  = 0;
   float pos    = 0;
   float noise  = 0;
   float high   = 0;
   float band   = 0;
   float low    = 0;
   int   inst;
   float pitch;
   int   length;
   int   arp_tick = 0;
   int   arp_env = 0;
};




std::array<Voice, POLY> voices;
std::array<Instrument, 128> instruments;
std::vector<float> envelops[] = {
   { 0, 0 },
   { -1, -0.5, 0, 2 },
   { -2, -2, -2, -2, 0, 0, 0, 0, -2, 8 },
};
void init_voices() {
   for (Voice& v : voices) v.state = OFF;
   instruments[ 0 ] = { 1,   PULSE, 0.5, 0,   { 0.01,  0.5, 0.9999,   0.9992 }  };
   instruments['l'] = { 0.8, PULSE, 0.2, 0.2, { 0.01,  0.3, 0.9999,   0.9992 }, 0.15, 0.09 };
   instruments['p'] = { 0.3, PULSE, 0.3,-0.3, { 0.001, 0.0, 0.999992, 0.9999 }, 0.3,  0.05 };
   instruments['b'] = { 1.8, PULSE, 0.3, 0.1,   { 0.01,  0.5, 0.9998,   0.9992 }, 0, 0.2, true, 1.1, -3.5 };
}


void play_voice(int inst, int note, int length, int arp_env) {
   Voice* v = nullptr;
   for (Voice& w : voices) {
      if (!v || w.state < v->state ||
         (w.state == v->state && w.level < v->level)) {
         v = &w;
      }
   }
   *v = Voice();
   ((Instrument&)*v) = instruments[inst];
   v->inst    = inst;
   v->pitch   = note;
   v->length  = length;
   v->arp_env = arp_env;
   if (inst == 'p') {
      v->pan += rand() / (float) RAND_MAX * 0.2 - 0.1;
      v->sweep += rand() / (float) RAND_MAX * 0.02 - 0.01;
   }
}


struct Track {
public:
   Track(const char* p) : pos(p) {}
   bool tick() {
      if (--wait > 0) return true;
      for (;;) {
         while (memchr(" \t", *pos, 2)) ++pos;
         if (*pos == '[') {
            loop_pos = ++pos;
            loop_count = 0;
            continue;
         }
         if (*pos == ']') {
            assert(loop_pos);
            ++pos;
            if (++loop_count < strtol(pos, (char**) &pos, 10)) pos = loop_pos;
            else loop_pos = nullptr;
            continue;
         }
         if (memchr("><", *pos, 2)) {
            state[OCT] += *pos++ == '>' ? 1 : -1;
            continue;
         }
         static const char* state_lut = "LOQA";
         if (const char* p = (const char*) memchr(state_lut, *pos, 4)) {
            ++pos;
            state[p - state_lut] = strtol(pos, (char**) &pos, 10);
            continue;
         }
         if (*pos == 'I') {
            ++pos;
            if (*pos) inst = *pos++;
            continue;
         }
         if (*pos == 'r') {
            ++pos;
            if (isdigit(*pos)) wait = strtol(pos, (char**) &pos, 10);
            else wait = state[LEN];
            return true;
         }
         static const char* note_lut = "ccddeffggaab";
         if (const char* p = (const char*) memchr(note_lut, *pos, 12)) {
            ++pos;
            int n = (p - note_lut) + state[OCT] * 12;
            while (memchr("-+", *pos, 2)) n += *pos++ == '+' ? 1 : -1;
            wait = strtol(pos, (char**) &pos, 10) ?: state[LEN];
            play_voice(inst, n, wait * SAMPLES_PER_ROW * state[QUANT] / 8, state[ARP]);
            if (*pos == '/') {
               ++pos;
               wait = 0;
               continue;
            }
            return true;
         }
         break;
      }
      return false;
   }
private:
   enum { LEN, OCT, QUANT, ARP };
   int state[5] = { 4, 4, 8, 0 };
   char inst = 0;
   int wait = 0;
   const char* pos;
   const char* loop_pos = nullptr;
   int loop_count;
};

struct Player {
public:
   Player() {
      skip();
      assert(*pos);
   }
   void tick() {
      for (auto it = tracks.begin(); it != tracks.end();) {
         if (it->tick()) ++it;
         else it = tracks.erase(it);
      }
      if (tracks.empty()) {
         while (*pos && *pos != '\n') {
            tracks.emplace_back(pos);
            tracks.back().tick();
            while (*pos && *pos != '\n') ++pos;
            if (*pos == '\n') ++pos;
            skip(false);
         }
         skip();
         if (!*pos) {
            pos = tune;
            skip();
         }
      }
   }
private:
   void skip(bool lines=true) {
      for (;;) {
         while (memchr("\n\t  " + !lines, *pos, 3)) ++pos;
         if (*pos != '#') break;
         while (*pos && *pos != '\n') ++pos;
         if (*pos == '\n') ++pos;
      }
   }
   const char* pos = tune;
   std::vector<Track> tracks;
} player;

class {
public:
   void add(const float* in) {
      buffer[pos][0] += in[1];
      buffer[pos][1] += in[0];
   }
   void mix(float* out) {
      pos = (pos + 1) % buffer.size();
      out[0] += buffer[pos][0] * 0.3;
      out[1] += buffer[pos][1] * 0.3;
      buffer[pos][0] *= 0.4;
      buffer[pos][1] *= 0.4;
   }
private:
   std::array<float[2], SAMPLES_PER_ROW * 3> buffer;
   int pos = 0;
} delay;

void mix(float* out) {
   static int sample = 0;
   if (sample == 0) player.tick();
   sample = (sample + 1) % SAMPLES_PER_ROW;
   for (Voice& v : voices) {
      if (++v.sample > v.length) v.state = OFF;
      switch (v.state) {
      case ATTACK:
         v.level += v.env[0];
         if (v.level > 1) v.state = HOLD;
         break;
      case HOLD:
         v.level = v.env[1] + (v.level - v.env[1]) * v.env[2];
         break;
      default:
         v.level *=  v.env[3];
         if (v.level <= 0.01) continue;
         break;
      }
      const auto& arp = envelops[v.arp_env];
      float offset = arp[v.arp_tick];
      if (sample % (SAMPLES_PER_ROW / 8) == 0) {
         if (++v.arp_tick + 1 >= (int) arp.size()) v.arp_tick = arp.back();
      }
      float pitch = v.pitch + offset + sinf(v.sample * 0.0008) * v.vib * (v.sample > 20000);
      float speed = exp2f((pitch - 57) / 12) * 440 / MIXRATE;
      v.pos = fmod(v.pos + speed, 1);
      v.pw = fmod(v.pw + v.sweep * 0.0001, 1);
      float amp = 0;
      switch (v.wave) {
      case PULSE: amp = v.pos > v.pw ? -1 : 1; break;
      case SAW:   amp = v.pos * 2 - 1; break;
      case TRI:   amp = v.pos < 0.5 ? v.pos * 4 - 1 : 3 - v.pos * 4; break;
      default: break;
      }
      if (v.filter) {
		 v.cutoff -= 0.0001;
         float f = exp2f(v.cutoff);
         v.low += f * v.band;
         v.high = amp - v.band * v.reso - v.low;
         v.band += f * v.high;
         amp = v.low;
      }

      float buf[2] = {
         amp * v.level * v.vol * sqrtf(0.5 - v.pan * 0.5),
         amp * v.level * v.vol * sqrtf(0.5 + v.pan * 0.5),
      };
      if (v.inst == 'l') delay.add(buf);
      out[0] += buf[0];
      out[1] += buf[1];
   }
   delay.mix(out);
}

void cb(void* u, Uint8* stream, int len) {
   short* buf = (short*) stream;
   for (; len > 0; len -= 4) {
      float out[2] = {};
      mix(out);
      *buf++ = std::max(-32768, std::min<int>(out[0] * 6000, 32767));
      *buf++ = std::max(-32768, std::min<int>(out[1] * 6000, 32767));
   }
}

int main() {
   init_voices();
   SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0, &cb };
   SDL_OpenAudio(&spec, nullptr);
   SDL_PauseAudio(0);
   getchar();
   SDL_Quit();
   return 0;
}
