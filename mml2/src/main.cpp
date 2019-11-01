#include "parser.hpp"
#include "synth.hpp"
#include <fstream>
#include <iostream>
#include <SDL2/SDL.h>



Tune tune;
Synth synth(tune);


int main(int argc, const char** argv) {
    if (argc != 2) {
        printf("usage: %s song\n", argv[0]);
        return 0;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        printf("error: cannot open file\n");
        return 1;
    }
    std::string content = {
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    Parser parser(content.c_str());
    parser.parse_tune(tune);

    SDL_AudioSpec spec = {
        MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0,
        [](void* u, Uint8* stream, int len) { synth.mix((short*) stream, len / 4); }
    };
    SDL_OpenAudio(&spec, nullptr);
    SDL_PauseAudio(0);
    printf("playing...\n");
    getchar();
    printf("done.\n");
    SDL_Quit();

    return 0;
}


/*

class Tune {
public:

    void init(const char* src) {
        m_src = m_pos = src;
        skip();
        m_src = m_pos;
    }

    void add_mix(Frame& out) {

        if (m_sample == 0) tick();
        if (++m_sample > m_samples_per_row) m_sample = 0;

        for (Voice& v : m_voices) {
            // TODO: don't hardcode tick length
            if (v.sample % 1200 == 0) {
                Inst& i = v.inst;
                for (Param& p : i.params) p.tick();
                v.attack  = powf(0.5, i.attack) * 0.01;
                v.sustain = clamp(i.sustain * 0.1);
                v.decay   = expf(logf(0.01) / MIXRATE * 10 / i.decay);
                v.release = expf(logf(0.01) / MIXRATE * 10 / i.release);
                if (i.pw.changed()) v.pw = i.pw * 0.1;
            }

            if (++v.sample > v.length) v.state = Voice::OFF;
            // envelope
            switch (v.state) {
            case Voice::ATTACK:
                v.level += v.attack;
                if (v.level > 1) v.state = Voice::HOLD;
                break;
            case Voice::HOLD:
                v.level = v.sustain + (v.level - v.sustain) * v.decay;
                break;
            default:
                v.level *=  v.release;
                if (v.level <= 0.01) continue;
                break;
            }

            // osc
            float pitch = v.pitch + v.inst.offset;
            pitch += sinf(v.sample * 0.0008) * v.inst.vib * 0.1 * (v.sample > 20000);
            float speed = exp2f((pitch - 57) / 12) * 440 / MIXRATE;
            v.pos += speed;
            v.pos -= floor(v.pos);

            // pulse sweep
            v.pw += v.inst.sweep * 0.000005;
            v.pw -= floor(v.pw);

            // wave
            float amp = 0;
            switch ((int) v.inst.wave) {
            case PULSE: amp = v.pos < v.pw ? -1 : 1; break;
            case TRIANGLE: amp = v.pos < v.pw ?
                2 / v.pw * v.pos - 1:
                2 / (v.pw - 1) * (v.pos - v.pw) + 1;
                break;
            case SINE: amp = sinf(v.pos * M_PI); break;
            case NOISE:
                if (v.pos < speed) v.noise = rand() * 2.0 / RAND_MAX - 1;
                amp = v.noise;
                break;
            default: break;
            }
            if (v.inst.filter) {
                float f = exp2f(v.inst.cutoff);
                v.low += f * v.band;
                v.high = amp - v.band * v.inst.reso - v.low;
                v.band += f * v.high;
                amp = v.low;
            }
            amp *= v.level * v.inst.vol * 0.1;
            Frame buf = {
                amp * sqrtf(0.5 - v.inst.pan * 0.05),
                amp * sqrtf(0.5 + v.inst.pan * 0.05)
            };
            out[0] += buf[0];
            out[1] += buf[1];
        }
    }

private:
    enum { PULSE, TRIANGLE, SINE, NOISE };

    void tick(Track& t) {
        if (!t.pos) return;
        --t.wait;
        while (t.wait <= 0) {
            t.pos += strspn(t.pos, "\t ");
            if (*t.pos == '[') {
                t.loop_pos = ++t.pos;
                t.loop_count = 0;
                continue;
            }
            if (*t.pos == ']') {
                if (!t.loop_pos) {
                    printf("track error: '[' missing\n");
                    break;
                }
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
                int i = p - state_lut;
                t.state[i] = strtoul(t.pos, (char**) &t.pos, 10);
                if (i == Track::INST && t.state[i] >= (int) m_insts.size()) {
                    printf("track error: invalid instrument selected\n");
                    t.state[i] = 0;
                }
                continue;
            }
            if (*t.pos == 'r') {
                ++t.pos;
                if (isdigit(*t.pos)) t.wait = strtoul(t.pos, (char**) &t.pos, 10);
                else t.wait = t.state[Track::LEN];
                continue;
            }

            bool legato = false;
            if (*t.pos == '~') {
                ++t.pos;
                legato = true;
            }
            static const char* note_lut = "ccddeffggaab";
            if (const char* p = (const char*) memchr(note_lut, *t.pos, 12)) {
                ++t.pos;
                int note = (p - note_lut) + t.state[Track::OCT] * 12;
                while (memchr("-+", *t.pos, 2)) note += *t.pos++ == '+' ? 1 : -1;
                t.wait = strtoul(t.pos, (char**) &t.pos, 10) ?: t.state[Track::LEN];

                if (legato && t.voice) {
                    t.voice->state = Voice::HOLD;
                }
                else {
                    t.voice = nullptr;
                    for (Voice& w : m_voices) {
                        if (!t.voice || w.state < t.voice->state
                        || (w.state == t.voice->state && w.level < t.voice->level)) t.voice = &w;
                    }
                    Voice& v = *t.voice;
                    if (v.track && v.track != &t) v.track->voice = nullptr;
                    v.track = &t;

                    if (t.state[Track::INST] < (int) m_insts.size()) {
                        v.inst = m_insts[t.state[Track::INST]];
                    }
                    else v.inst = Inst();
                    v.state  = Voice::ATTACK;
                    v.sample = 0;
                    v.level  = 0;
                    v.pos    = 0;
                    v.high   = 0;
                    v.band   = 0;
                    v.low    = 0;
                }
                Voice& v = *t.voice;
                v.sample = 0;
                v.length = t.wait * m_samples_per_row * t.state[Track::QUANT] / 8;
                v.pitch  = note;

                // parse instrument settings
                if (*t.pos == '(') {
                    ++t.pos;
                    parse_inst_settings(t.pos, v.inst);
                    if (*t.pos != ')') {
                        printf("track error: ')' expected\n");
                    }
                    ++t.pos;
                }

                // polyphony
                if (*t.pos == '/') {
                    ++t.pos;
                    t.wait = 0;
                }
                continue;
            }

            // can't parse any further
            if (*t.pos != '\n' && *t.pos != '#') {
                printf("track error\n");
            }
            t.pos = nullptr;
            break;
        }
    }

    void tick() {
        bool loop = false;
        for (;;) {
            bool idle = true;
            for (Track& t : m_tracks) {
                tick(t);
                if (t.wait > 0 && t.pos) idle = false;
            }
            if (!idle) break;
            if (loop) {
                printf("song error\n");
                break;
            }
            for (int i = 0; i < (int) m_tracks.size() && *m_pos && *m_pos != '\n';) {
                if (*m_pos == '@') parse_env();
                else if (*m_pos == ':') parse_inst();
                else {
                    Track& t     = m_tracks[i++];
                    t.pos        = m_pos;
                    t.loop_pos   = nullptr;
                    t.loop_count = 0;
                    t.state      = { 4, 4, 7, 0 };
                    t.wait       = 0;
                }
                // next line
                while (*m_pos && *m_pos != '\n') ++m_pos;
                m_pos += (*m_pos == '\n');
                skip(false);
            }
            skip();
            if (!*m_pos || *m_pos == '-') {
                m_pos = m_src;
                loop = true;
            }
        }
    }

    std::array<Track, TRACK_COUNT> m_tracks;

    std::vector<Env>               m_envs;
    std::vector<Inst>              m_insts{1};

    int                            m_samples_per_row = 5500;
    int                            m_sample = 0;
};


Tune tune;


void callback(void* u, Uint8* stream, int len) {
    short* buf = (short*) stream;
    for (; len > 0; len -= 4) {
        Frame f = {};
        tune.add_mix(f);
        *buf++ = clamp<int>(f[0] * 7000, -32768, 32767);
        *buf++ = clamp<int>(f[1] * 7000, -32768, 32767);
    }
}

int main(int argc, const char** argv) {
    if (argc != 2) {
        printf("usage: %s song\n", argv[0]);
        return 0;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        printf("file error\n");
        return 1;
    }
    std::string content{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    tune.init(content.c_str());

    SDL_AudioSpec spec = { MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0, &callback };
    SDL_OpenAudio(&spec, nullptr);
    SDL_PauseAudio(0);
    printf("playing...\n");
    getchar();
    printf("done.\n");
    SDL_Quit();
    return 0;
}
*/
