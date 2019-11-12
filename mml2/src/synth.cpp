#include "synth.hpp"
#include <cmath>


template <class T>
T clamp(T const& v, T const& min=0, T const& max=1) { return std::max(min, std::min(max, v)); }



void Synth::tick() {
    enum { FRAMES_PER_ROW = 6 };

    // new row
    if (m_frame == 0) {
        for (int i = 0; i < CHANNEL_COUNT; ++i) {
            Track const& track = m_tune.tracks[i];
            if (track.events.empty()) continue;
            Channel& chan = m_channels[i];

            if (chan.length > 0) {
                if (--chan.length == 0) {
                    chan.state = Channel::S_RELEASE;
                }
            }

            while (chan.wait == 0) {
                Track::Event const& e = track.events[chan.pos];
                if (++chan.pos >= (int) track.events.size()) chan.pos = 0;

                chan.wait   = e.wait;
                chan.length = e.length;
                if (e.note != -1) {
                    chan.note = e.note;
                    chan.state = Channel::S_ATTACK;
                }
                else {
                    chan.state = Channel::S_RELEASE;
                }

                if (e.inst_nr >= 0 && e.note >= 0) {
                    Inst const& inst = m_tune.insts[e.inst_nr];
                    for (int i = 0; i < Inst::PARAM_COUNT; ++i) {
                        chan.params[i].set(&inst.params[i]);
                    }
                }
            }
            if (chan.wait > 0) --chan.wait;


            if (chan.wait == 0 && !track.events.empty() && track.events[chan.pos].note != -1) {
                chan.kill = FRAMES_PER_ROW - 2;
            }
            else {
                chan.kill = FRAMES_PER_ROW;
            }
        }
    }

    for (Channel& chan : m_channels) {
        for (Param& p : chan.params) p.tick();

        chan.wave = (Channel::Wave) chan.params[Inst::P_WAVE].value();

        float pw = chan.params[Inst::P_PULSEWIDTH].value();
        chan.next_pulsewidth = 0.5f + std::abs(pw - std::floor(pw) - 0.5f) * 0.97f;

        float vol = clamp(chan.params[Inst::P_VOLUME].value());
        float pan = clamp(chan.params[Inst::P_PANNING].value(), -1.0f, 1.0f) * 0.5;
        chan.panning[0] = std::sqrt(0.5f - pan) * vol;
        chan.panning[1] = std::sqrt(0.5f + pan) * vol;

        chan.attack  = 1.0f / std::max(0.0f, chan.params[Inst::P_ATTACK ].value() * 0.001f * MIXRATE);
        chan.decay   = 1.0f / std::max(0.0f, chan.params[Inst::P_DECAY  ].value() * 0.001f * MIXRATE);
        chan.release = 1.0f / std::max(0.0f, chan.params[Inst::P_RELEASE].value() * 0.001f * MIXRATE);
        chan.sustain = clamp(chan.params[Inst::P_SUSTAIN].value());

        if (m_frame >= chan.kill) {
            chan.state   = Channel::S_RELEASE;
            chan.release = 1.0f / (0.01f * MIXRATE);
            chan.sustain = 1.0f / (0.01f * MIXRATE);
        }

        if (chan.pos == 0 && chan.wait == 0 && m_frame == FRAMES_PER_ROW - 1) {
            ++chan.loop_count;
        }
    }

    if (++m_frame >= FRAMES_PER_ROW) m_frame = 0;
}


bool Synth::done() const {
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (m_channels[i].loop_count == 0) return false;
    }
    return true;
}


void Synth::mix(int16_t* buffer, int len) {
    for (int i = 0; i < len; ++i) {
        if (m_sample == 0) tick();
        if (++m_sample >= FRAME_LENGTH) m_sample = 0;

        float f[2] = { 0, 0 };

        for (Channel& chan : m_channels) {

            // adsr
            switch (chan.state) {
            case Channel::S_ATTACK:
                chan.level += chan.attack;
                if (chan.level >= 1) {
                    chan.level = 1;
                    chan.state = Channel::S_HOLD;
                }
                break;
            case Channel::S_HOLD:
                chan.level = clamp(chan.sustain, chan.level - chan.decay, chan.level + chan.decay);
                break;
            case Channel::S_RELEASE:
                chan.level -= chan.release;
                if (chan.level < 0) chan.level = 0;
                break;
            }

            // osc
            float pitch = chan.note - 57 + chan.params[Inst::P_PITCH].value();
            float speed = std::exp2(pitch / 12.0f) * 440 / MIXRATE;
            chan.phase += speed;
            chan.phase -= (int) chan.phase;
            if (chan.phase < speed) chan.pulsewidth = chan.next_pulsewidth;

            // wave
            float amp = 0;
            switch (chan.wave) {
            case Channel::W_NOISE: {
                int n = int(chan.phase * 32);
                if (chan.noise_phase != n) {
                    chan.noise_phase = n;
                    uint32_t s = chan.shift;
                    chan.shift = s = (s << 1) | (((s >> 22) ^ (s >> 17)) & 1);
                    uint8_t a = ((s & 0x400000) >> 11) |
                                ((s & 0x100000) >> 10) |
                                ((s & 0x010000) >>  7) |
                                ((s & 0x002000) >>  5) |
                                ((s & 0x000800) >>  4) |
                                ((s & 0x000080) >>  1) |
                                ((s & 0x000010) <<  1) |
                                ((s & 0x000004) <<  2);
                    chan.noise = a / 128.0f - 1;
                }
                amp = chan.noise;
                break;
            }
            case Channel::W_PULSE:    amp = chan.phase < chan.pulsewidth ? -1 : 1; break;
            case Channel::W_SAW:      amp = chan.phase * 2 - 1; break;
            case Channel::W_TRIANGLE: amp = chan.phase < 0.5 ? 4 * chan.phase - 1 : -4 * (chan.phase - 0.5) + 1; break;
            case Channel::W_SINE:     amp = -std::cos(chan.phase * 2 * (float) M_PI); break;
            default: break;
            }

            amp *= chan.level;
            f[0] += amp * chan.panning[0];
            f[1] += amp * chan.panning[1];
        }

        *buffer++ = clamp<int>(f[0] * 7000, -32768, 32767);
        *buffer++ = clamp<int>(f[1] * 7000, -32768, 32767);
    }
}

