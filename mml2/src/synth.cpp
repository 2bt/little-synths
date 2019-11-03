#include "synth.hpp"
#include <cmath>


template <class T>
T clamp(T const& v, T const& min=0, T const& max=1) { return std::max(min, std::min(max, v)); }



void Synth::tick() {
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        Channel& chan = m_channels[i];
        Track const& track = m_tune.tracks[i];

        while (chan.wait == 0) {
            if (chan.pos >= (int) track.events.size()) break;
            Track::Event const& e = track.events[chan.pos++];
            chan.wait = e.length;
            chan.note = e.note;


            if (e.inst_nr >= 0) chan.inst = &m_tune.insts[e.inst_nr];
            printf("%d\n", e.note);
        }
        if (chan.wait > 0) --chan.wait;
    }
}

void Synth::mix(int16_t* buffer, int len) {

    for (int i = 0; i < len; ++i) {

        if (m_sample == 0) tick();
        if (++m_sample >= FRAME_LENGTH * 6) m_sample = 0;


        float f[2] = {};


        for (Channel& chan : m_channels) {
            if (chan.note == -1) continue;

            float speed = std::exp2((chan.note - 57) / 12.0f) * 440 / MIXRATE;
            chan.phase += speed;
            chan.phase -= (int) chan.phase;

            float amp = chan.phase < 0.5 ? -1 : 1;
            f[0] += amp;
            f[1] += amp;
        }

        *buffer++ = clamp<int>(f[0] * 7000, -32768, 32767);
        *buffer++ = clamp<int>(f[1] * 7000, -32768, 32767);
    }
}

