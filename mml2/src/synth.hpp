#pragma once

#include "tune.hpp"


class Param {
public:
    void set(Env const* e) {
        m_env = e;
        m_pos = -1;
    }
    float value() const { return m_val; }
    void tick() {
        if (m_pos == -1) {
            m_pos = 0;
        }
        if (!m_env || m_env->data.empty()) return;
        if (m_env->data[m_pos].relative) m_val += m_env->data[m_pos].value;
        else                             m_val =  m_env->data[m_pos].value;
        if (++m_pos >= (int) m_env->data.size()) m_pos = m_env->loop;
    }
private:
    Env const* m_env;
    int        m_pos;
    float      m_val = 0;
};


struct Channel {
    int         loop_count;
    int         pos;
    int         wait;
    int         note;
    Inst const* inst;

    // params
    std::array<Param, Inst::PARAM_COUNT> params;

    // param cache
    enum Wave { W_NOISE, W_PULSE, W_SAW, W_TRIANGLE, W_SINE };
    Wave   wave;
    float  pulsewidth;
    float  next_pulsewidth;
    float  panning[2];

    float  attack;
    float  sustain;
    float  decay;
    float  release;


    // voice stuff
    enum State { S_RELEASE, S_ATTACK, S_DECAY, S_SUSTAIN };
    float    noise;
    uint32_t shift = 0x7ffff8;
    int      noise_phase;
    float    phase;
    State    state;
    float    level;
    int      length;
};

struct Filter {
    float  high;
    float  band;
    float  low;
};


class Synth {
public:
    Synth(Tune const& tune) : m_tune(tune) {}
    void mix(int16_t* buffer, int len);
    bool done() const;
private:
    void tick();

    int m_sample = 0;
    int m_frame  = 0;

    // XXX: filter, echo, ...
    Filter                             m_filter;


    Tune const&                        m_tune;
    std::array<Channel, CHANNEL_COUNT> m_channels;
};

