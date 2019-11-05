#include "synth.hpp"
#include <cmath>


template <class T>
T clamp(T const& v, T const& min=0, T const& max=1) { return std::max(min, std::min(max, v)); }



void Synth::tick() {

    // new row
    if (m_frame == 0) {
        for (int i = 0; i < CHANNEL_COUNT; ++i) {
            Channel& chan = m_channels[i];
            Track const& track = m_tune.tracks[i];

            while (chan.wait == 0) {
                Track::Event e = { -1, 1, -1 };
                if (chan.pos < (int) track.events.size()) e = track.events[chan.pos++];

                chan.wait = e.length;
                chan.note = e.note;

                if (e.inst_nr >= 0) chan.inst = &m_tune.insts[e.inst_nr];
                if (chan.inst) {
                    for (int i = 0; i < Inst::PARAM_COUNT; ++i) {
                        chan.params[i].set(&chan.inst->params[i]);
                    }
                }
            }
            if (chan.wait > 0) --chan.wait;
        }
    }
    enum { FRAMES_PER_ROW = 6 };
    if (++m_frame >= FRAMES_PER_ROW) m_frame = 0;

    for (Channel& chan : m_channels) {
        for (Param& p : chan.params) p.tick();
        // param cache
        chan.wave = (int) chan.params[Inst::P_WAVE].value();

        float pw = chan.params[Inst::P_PULSEWIDTH].value();
        pw -= (int) pw;
        chan.next_pulsewidth = 0.5f + std::abs(pw - 0.5f) * 0.97f;
    }
}


bool Synth::done() const {
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        Channel const& chan = m_channels[i];
        Track const& track = m_tune.tracks[i];
        if (chan.pos < (int) track.events.size() || chan.wait > 0) return false;
    }
    return true;
}


void Synth::mix(int16_t* buffer, int len) {

    for (int i = 0; i < len; ++i) {

        if (m_sample == 0) tick();
        if (++m_sample >= FRAME_LENGTH) m_sample = 0;


        float f[2] = { 0, 0 };


        for (Channel& chan : m_channels) {
            if (chan.note == -1) continue;

            float pitch = chan.note - 57 + chan.params[Inst::P_PITCH].value();
            float speed = std::exp2(pitch / 12.0f) * 440 / MIXRATE;
            chan.phase += speed;
            chan.phase -= (int) chan.phase;
            if (chan.phase < speed) chan.pulsewidth = chan.next_pulsewidth;

            float amp = 0;
            switch (chan.wave) {
            case 0:
//                if (chan.phase < speed) chan.phase = rand() * 2.0 / RAND_MAX - 1;
//                amp = v.noise;
                break;
            case 1: amp = chan.phase < chan.pulsewidth ? -1 : 1; break;
            case 2: amp = chan.phase * 2 - 1; break;
            case 3: amp = chan.phase < 0.5 ? 4 * chan.phase - 1 : -4 * (chan.phase - 0.5) + 1; break;
            case 4: amp = -std::cos(chan.phase * 2 * (float) M_PI); break;
            default: break;
            }

            /*
            amp *= v.level * v.inst.vol * 0.1;
            Frame buf = {
                amp * sqrtf(0.5 - v.inst.pan * 0.05),
                amp * sqrtf(0.5 + v.inst.pan * 0.05)
            };
            out[0] += buf[0];
            out[1] += buf[1];
            if (v.inst.echo > 0) {
                m_echo.add({
                    buf[0] * v.inst.echo * 0.1f,
                    buf[1] * v.inst.echo * 0.1f
                });
            }
            */

            f[0] += amp;
            f[1] += amp;
        }

        *buffer++ = clamp<int>(f[0] * 7000, -32768, 32767);
        *buffer++ = clamp<int>(f[1] * 7000, -32768, 32767);
    }
}

