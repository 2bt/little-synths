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

ill2> e12def4ec4d4e14def4ec4d4d18 c<g>cdedc<g>e16f8g8
q4<< ccrr<a+a+rraarrg+g+rr ggrr >ddrrccrr<f>f<g>g
ipl16q7 <g/>c/e <g+/>c/f <g/>c/e <g+/>c/f <a+/>d/f <a/>c/f <g/>c/e l8<f/a/>c <g/b/>d

ill2> e12def4ec4d4e14def4ec4d4d18 c<g>cdedc<g>e16f8g8
q4<< ccrr<a+a+rraarrg+g+rr ggrr >ddrrccrr<f>f<g>g
ipl16q7 <g/>c/e <g+/>c/f <g/>c/e <g+/>c/f <a+/>d/f <a/>c/f <g/>c/e l8<f/a/>c <g/b/>d

r

q4l2gga+gggfggga+>drg<gf>
l4>crcrcrcc
l4>d+rdrfrd+g2f2

q4l1o4{cccg2ccf2ccd+2cd2}3cccg2ccf2ccd+2ca+>c
o2l4q3{c>ccc<}2q7l2{c<c>>c<c<c>ca+>c<}2

o2l4q3{c>ccc<}2q7l2{c<c>>c<c<c>ca+>c<}2
q8r>c<d6d+2crdd+f6d+1d1d+6d1c1d8<a+>c

r

)";

enum {
   MIXRATE         = 48000,
   POLY            = 32,
   SAMPLES_PER_ROW = 5500,
};
enum State { OFF, HOLD, ATTACK };
enum Wave { PULSE, SAW };
struct Instrument {
   float vol;
   Wave  wave;
   float pw;
   float pan;
   float env[4];
   float vib;
   float sweep;
};
struct Voice : Instrument {
   int   sample = 0;
   State state  = ATTACK;
   float level  = 0;
   float pos    = 0;
   float noise  = 0;
   int   inst;
   float pitch;
   int   length;
};

std::array<Voice, POLY> voices;
std::array<Instrument, 128> instruments;

void init_voices() {
   for (Voice& v : voices) v.state = OFF;
   instruments[ 0 ] = { 1, PULSE, 0.5, 0, { 0.01, 0.5, 0.9998, 0.9992 }, 0, 0 };
   instruments['l'] = { 0.9, PULSE, 0.5, 0.1, { 0.01, 0.5, 0.9998, 0.9992 }, 0, 0 };
   instruments['p'] = { 0.4, PULSE, 0.3, 0, { 0.001, 0.8, 0.9998, 0.9999 }, 0.2, 0.05 };
}


void play_voice(int inst, int note, int length) {
   Voice* v = nullptr;
   for (Voice& w : voices) {
      if (!v || w.state < v->state ||
         (w.state == v->state && w.level < v->level)) {
         v = &w;
      }
   }
   *v = Voice();
   ((Instrument&)*v) = instruments[inst];
   v->inst   = inst;
   v->pitch  = note;
   v->length = length;
}


struct Track {
public:
   Track(const char* p) : pos(p) {}
   bool tick() {
      if (--wait > 0) return true;
      for (;;) {
         while (memchr(" \t", *pos, 2)) ++pos;
         if (*pos == '{') {
            ++pos;
            loop = 0;
            continue;
         }
         if (*pos == '}') {
            assert(loop != -1);
            ++pos;
            if (++loop < strtol(pos, (char**) &pos, 10)) {
               while (pos[-1] != '{') --pos;
            }
            else loop = -1;
            continue;
         }
         if (memchr("><", *pos, 2)) {
            state[OCT] += *pos++ == '>' ? 1 : -1;
            continue;
         }
         static const char* state_lut = "loq";
         if (const char* p = (const char*) memchr(state_lut, *pos, 4)) {
            ++pos;
            if (isdigit(*pos)) state[p - state_lut] = strtol(pos, (char**) &pos, 10);
            continue;
         }
         if (*pos == 'i') {
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
            play_voice(inst, n, wait * SAMPLES_PER_ROW * state[QUANT] / 8);
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
   enum { LEN, OCT, QUANT };
   int state[4] = { 4, 4, 8 };
   char inst = 0;
   const char* pos;
   int wait = 0;
   int loop = -1;
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
      float pitch = v.pitch + sinf(v.sample * 0.0008) * v.vib * (v.sample > 20000);
      float speed = exp2f((pitch - 57) / 12) * 440 / MIXRATE;
      v.pos = fmod(v.pos + speed, 1);
      v.pw = fmod(v.pw + v.sweep * 0.0001, 1);
      float amp = 0;
      switch (v.wave) {
      case PULSE: amp = v.pos > v.pw ? -1 : 1; break;
      case SAW:   amp = v.pos * 2 - 1; break;
      default: break;
      }
      out[0] += amp * v.level * v.vol * sqrtf(0.5 - v.pan * 0.5);
      out[1] += amp * v.level * v.vol * sqrtf(0.5 + v.pan * 0.5);
   }
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
