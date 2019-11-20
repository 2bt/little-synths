#include "synth.hpp"
#include <cmath>


template <class T>
T clamp(T const& v, T const& min=0, T const& max=1) { return std::max(min, std::min(max, v)); }

void Synth::init() {

    static Env envs[] = {
        { { { false,  1 } }, 0 },
        { { { false, 15 } }, 0 },
        { { { false, 10 } }, 0 },
    };
    m_params[Inst::G_FILTER_TYPE].set(&envs[0]);
    m_params[Inst::G_FILTER_RESO].set(&envs[1]);
    m_params[Inst::G_FILTER_FREQ].set(&envs[2]);


    for (int n = 0; n < CHANNEL_COUNT; ++n) {
        Track const& track = m_tune.tracks[n];
        if (track.events.empty()) continue;
        Channel& chan = m_channels[n];
        chan.pos = track.start % track.events.size();
    }
}


void Synth::tick() {
    enum { FRAMES_PER_ROW = 6 };

    // new row
    if (m_frame == 0) {
        for (int n = 0; n < CHANNEL_COUNT; ++n) {
            Track const& track = m_tune.tracks[n];
            if (track.events.empty()) continue;
            Channel& chan = m_channels[n];

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

                if (e.inst_nr >= 0) {
                    Inst const& inst = m_tune.insts[e.inst_nr];

                    // params
                    for (int i = 0; i < Inst::PARAM_COUNT_TOTAL; ++i) {
                        if (inst.envs[i].data.empty()) continue;
                        if (i < Inst::PARAM_COUNT_LOCAL) chan.params[i].set(&inst.envs[i]);
                        else m_params[i - Inst::PARAM_COUNT_LOCAL].set(&inst.envs[i]);
                    }
                }
            }
            if (chan.wait > 0) --chan.wait;


            if (chan.wait == 0 && !track.events.empty() && track.events[chan.pos].note != -1) {
                chan.break_frame = FRAMES_PER_ROW - (int) chan.params[Inst::L_BREAK].value();
            }
            else {
                chan.break_frame = FRAMES_PER_ROW;
            }
        }
    }

    for (Channel& chan : m_channels) {
        for (Param& p : chan.params) p.tick();

        float pitch = chan.note - 57 + chan.params[Inst::L_PITCH].value();
        chan.speed = std::exp2(pitch / 12.0f) * 440 / MIXRATE;

        chan.wave = (Channel::Wave) chan.params[Inst::L_WAVE].value();
        float pw  = chan.params[Inst::L_PULSEWIDTH].value();
        chan.next_pulsewidth = 0.5f + std::abs(pw - std::floor(pw) - 0.5f) * 0.97f;

        float vol = clamp(chan.params[Inst::L_VOLUME].value());
        float pan = clamp(chan.params[Inst::L_PANNING].value(), -1.0f, 1.0f) * 0.5;
        chan.panning[0] = std::sqrt(0.5f - pan) * vol;
        chan.panning[1] = std::sqrt(0.5f + pan) * vol;

        chan.filter = clamp(chan.params[Inst::L_FILTER].value());

        chan.attack  = 1.0f / std::max(0.0f, chan.params[Inst::L_ATTACK ].value() * 0.001f * MIXRATE);
        chan.decay   = 1.0f / std::max(0.0f, chan.params[Inst::L_DECAY  ].value() * 0.001f * MIXRATE);
        chan.release = 1.0f / std::max(0.0f, chan.params[Inst::L_RELEASE].value() * 0.001f * MIXRATE);
        chan.sustain = clamp(chan.params[Inst::L_SUSTAIN].value());

        if (chan.params[Inst::L_GATE].value() == 0) chan.state = Channel::S_RELEASE;

        if (m_frame >= chan.break_frame) {
            chan.state   = Channel::S_RELEASE;
            chan.release = 1.0f / (0.01f * MIXRATE);
            chan.sustain = 1.0f / (0.01f * MIXRATE);
        }
        if (chan.pos == 0 && chan.wait == 0 && m_frame == FRAMES_PER_ROW - 1) {
            ++chan.loop_count;
        }
    }

    m_params[Inst::G_FILTER_TYPE].tick();
    m_params[Inst::G_FILTER_FREQ].tick();
    m_params[Inst::G_FILTER_RESO].tick();

    m_filter.type = clamp((int) m_params[Inst::G_FILTER_TYPE].value(), 0, 7);
    float f = clamp(m_params[Inst::G_FILTER_FREQ].value(), 0.0f, 100.0f);
    float r = clamp(m_params[Inst::G_FILTER_RESO].value(), 0.0f, 15.0f);

    m_filter.freq = f * (441.0f / MIXRATE);
    m_filter.reso = 1.2f - 0.04f * r;

    if (++m_frame >= FRAMES_PER_ROW) m_frame = 0;
}


bool Synth::tune_done() const {
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (m_channels[i].loop_count == 0) return false;
    }
    return true;
}


void Synth::mix(int16_t* buffer, int len) {
    for (int i = 0; i < len; ++i) {
        if (m_sample == 0) tick();
        if (++m_sample >= FRAME_LENGTH) m_sample = 0;

        float f[2][2] = {};

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
            chan.phase += chan.speed;
            chan.phase -= (int) chan.phase;
            if (chan.phase < chan.speed) chan.pulsewidth = chan.next_pulsewidth;

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
            f[0][0] += amp * chan.panning[0] * (1 - chan.filter);
            f[0][1] += amp * chan.panning[1] * (1 - chan.filter);
            f[1][0] += amp * chan.panning[0] * chan.filter;
            f[1][1] += amp * chan.panning[1] * chan.filter;
        }

        m_filter.high[0] = f[1][0] - m_filter.band[0] * m_filter.reso - m_filter.low[0];
        m_filter.high[1] = f[1][1] - m_filter.band[1] * m_filter.reso - m_filter.low[1];
        m_filter.band[0] += m_filter.freq * m_filter.high[0];
        m_filter.band[1] += m_filter.freq * m_filter.high[1];
        m_filter.low[0]  += m_filter.freq * m_filter.band[0];
        m_filter.low[1]  += m_filter.freq * m_filter.band[1];

        if (m_filter.type & Filter::T_LOW) {
            f[0][0] += m_filter.low[0];
            f[0][1] += m_filter.low[1];
        }
        if (m_filter.type & Filter::T_BAND) {
            f[0][0] += m_filter.band[0];
            f[0][1] += m_filter.band[1];
        }
        if (m_filter.type & Filter::T_HIGH) {
            f[0][0] += m_filter.high[0];
            f[0][1] += m_filter.high[1];
        }

        *buffer++ = clamp<int>(f[0][0] * 7000, -32768, 32767);
        *buffer++ = clamp<int>(f[0][1] * 7000, -32768, 32767);
    }
}

