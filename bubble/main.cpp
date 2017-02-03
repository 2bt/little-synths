#if 0
g++ -Wall -std=c++11 -O3 -xc++ $0 -lSDL2 && ./a.out
rm -f a.out
exit
#endif

#include <cstdlib>
#include <algorithm>
#include <vector>
#include <array>
#include <SDL2/SDL.h>

enum {
   MIXRATE = 44100,
   COLS    = 120,
   POLY    = 32,
};

struct Tune {
   typedef std::array<char, COLS>   Row;
   typedef std::vector<Row>         Pattern;
   std::vector<Pattern>             patterns;
   std::vector<std::vector<int>>    table;
   void load() {
      FILE* f = fopen("tune", "r");
      if (!f) return;
      table.clear();
      patterns.clear();
      Tune::Row r;
      for (;;) {
         if (!fgets(r.data(), r.size(), f)) break;
         if (r[0] == '#') break;
         table.emplace_back();
         char* q = r.data();
         while (char* p = strsep(&q, " \t")) {
            if (q - p != 1) table.back().emplace_back(atoi(p));
         }
      }
      patterns.emplace_back();
      for (;;) {
         memset(r.data(), 0, COLS);
         if (!fgets(r.data(), r.size(), f)) break;
         if (r[0] == '#') {
            patterns.emplace_back();
            continue;
         }
         patterns.back().emplace_back(r);
      }
      fclose(f);
   }
} tune;

enum State { OFF, HOLD, ATTACK };
struct Voice {
   int   slot;
   char  inst;
   float pitch;
   float high   = 0;
   float band   = 0;
   float low    = 0;
   int   sample = 0;
   State state  = ATTACK;
   float level  = 0;
   float pos    = 0;
   float noise  = 0;
   // default sound
   float vol    = 1;
   float pw     = 0.5;
   float pan    = 0;
   float env[4] = { 0.01, 0.5, 0.9998, 0.9994 };
   float vib    = 0;
   float sweep  = 0;
   bool  filter = false;
   float reso   = 0.2;
   float cutoff = 0;
};

std::array<Voice, POLY>       voices;
std::array<Voice*, COLS * 16> voice_slot_map;

Voice& get_voice() {
   Voice* voice = nullptr;
   for (Voice& v : voices) {
      if (!voice || v.state < voice->state ||
         (v.state == voice->state && v.level < voice->level)) {
         voice = &v;
      }
   }
   return *voice;
}

int   block = 0;
int   row = 0;
int   sample = 0;

void tick() {
   for (Voice& v : voices) v.state = OFF;
   for (int j = 0; j < (int) tune.table[block].size(); ++j) {
      int p = tune.table[block][j];
      auto & pat = tune.patterns[p];
      if (row >= (int) pat.size()) continue;
      for (int n = 0; n < COLS; ++n) {
         char i = pat[row][n];
         if (i <= 32) continue;
         int slot = j + COLS + n;
         if (i == '|') {
            if (Voice* v = voice_slot_map[slot]) v->state = HOLD;
            continue;
         }
         Voice& v  = get_voice();
         if (voice_slot_map[v.slot] == &v) {
            voice_slot_map[v.slot] = nullptr;
         }
         voice_slot_map[slot] = &v;
         v       = Voice();
         v.slot  = slot;
         v.inst  = i;
         v.pitch = n;
         if (v.inst == 'l') {
            v.pw    = 0.1;
            v.vol   = 0.9;
            v.vib   = 0.6;
            v.pan   = 0.1;
            v.sweep = 0.00003;
         }
         if (v.inst == 'a') {
            v.filter = true;
            v.cutoff = -2.5 - sinf(row * 0.1 + 0.9) * 2.5;
            v.reso   = 0.2;
            v.pan    = -0.1;
            v.pw     = row * 0.01;
            v.sweep  = 0.00002;
         }
         if (v.inst == 'n') {
            v.pan    = rand() / float(RAND_MAX) - 0.5;
            v.env[2] = v.env[3] = 0.99975;
         }
         if (v.inst == 'b') {
            v.filter = true;
            v.cutoff = 0.7;
            v.reso   = 0.4;
            v.vib    = 0.7;
            v.vol    = 0.6;
            v.pw     = 0.1 + rand() / float(RAND_MAX) * 0.2;
            v.env[0] = 0.0005;
            v.env[1] = 0.8;
            v.env[2] = v.env[3] = 0.9998;
            v.pan    = rand() / float(RAND_MAX) - 0.5;
         }
         if (v.inst == '+') {
            v.vol    = 1.1;
            v.pan    = 0.1;
            v.pitch += 20;
         }
         if (v.inst == '*') {
            v.vol    = 1.1;
            v.pan    = -0.3;
            v.pitch += 30;
            v.env[1] = 0;
            v.env[2] = v.env[3] = 0.99965;
         }
      }
   }
   //printf("%02X:%02X\n", block, row);
   //for (Voice& v : voices) printf("%d", v.state);
   //printf("\n");
}

void mix(float* out) {
   if (sample == 0) tick();
   for (Voice& v : voices) {
      // envelop
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
      // waveform
      ++v.sample;
      float pitch = v.pitch;
      pitch += sinf(v.sample * 0.001) * v.vib * (v.sample > 20000);
      if (v.inst == 'l') {
         pitch += + (v.sample < 1000) * 5;
      }
      float speed = exp2f((pitch - 33) / 12) * 440 / MIXRATE;
      v.pos += speed;
      v.pos -= (int) v.pos;
      v.pw += v.sweep;
      v.pw -= (int) v.sweep;
      float amp = 0;
      if (v.inst == 'b') {
         amp = v.pos * 2 - 1;
      }
      else if (v.inst == '+') {
         amp = (rand() / float(RAND_MAX) - 0.5) * 0.5;
         amp += atanf(sinf(v.pos * 2 * M_PI) * 3) * 1.5;
         v.pitch -= 0.007;
      }
      else if (v.inst == '*') {
         if (fmod(v.pos, 0.03) < speed) v.noise = (rand() / float(RAND_MAX) - 0.5) * 2;
         amp = v.noise;
         amp += atanf(sinf(v.pos * 2 * M_PI) * 3) * 1.2;
         v.pitch *= 0.9997;
         v.pitch += 0.003;
      }
      else if (v.inst == 'n') {
         if (v.pos < speed) v.noise = (rand() / float(RAND_MAX) - 0.5) * 2;
         amp = v.noise;
      }
      else {
         amp = v.pos > v.pw ? -1 : 1;
      }
      // filter
      if (v.filter) {
         if (v.inst == 'a') v.cutoff -= 0.0002;
         float f = exp2f(v.cutoff);
         v.low += f * v.band;
         v.high = amp - v.band * v.reso - v.low;
         v.band += f * v.high;
         amp = v.low;
      }
      out[0] += amp * v.level * v.vol * sqrtf(0.5 - v.pan * 0.5);
      out[1] += amp * v.level * v.vol * sqrtf(0.5 + v.pan * 0.5);
   }
   if (++sample >= 5000) {
      sample = 0;
      if (++row >= (int) tune.patterns[tune.table[block][0]].size()) {
         row = 0;
         tune.load();
         if (++block >= (int) tune.table.size()) {
            block = 0;
         }
      }
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
   tune.load();
   SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0, &cb };
   SDL_OpenAudio(&spec, nullptr);
   SDL_PauseAudio(0);
   getchar();
   SDL_Quit();
   return 0;
}
