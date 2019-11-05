#pragma once

#include "tune.hpp"


struct Param {
public:
    void set(const Env& e) {
        m_val = 0;
        m_env = &e;
        m_pos = -1;
    }
    float value() const { return m_val; }
    void tick() {
        m_changed = false;
        if (m_pos == -1) {
            m_pos = 0;
            m_changed = true;
        }
        if (!m_env || m_env->data.empty()) return;
        float v = m_val;
        if (m_env->data[m_pos].relative) m_val += m_env->data[m_pos].value;
        else                             m_val =  m_env->data[m_pos].value;
        if (v != m_val) m_changed = true;
        if (++m_pos >= (int) m_env->data.size()) m_pos = m_env->loop;
    }
    bool changed() const { return m_changed; }
private:
    const Env* m_env;
    int        m_pos;
    float      m_val;
    bool       m_changed;
};


struct Channel {
    int         pos  = 0;
    int         wait = 0;
    int         note = -1;
    Inst const* inst;

    // params
    std::array<Param, Inst::PARAM_COUNT> params;

    // param cache
    int    wave;
    float  attack;
    float  sustain;
    float  decay;
    float  release;
    float  pulsewidth;


    // voice stuff
    float  phase;
    enum State { OFF, HOLD, ATTACK };
    State  state = OFF;
    int    length;
    int    sample;
    float  level;
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

