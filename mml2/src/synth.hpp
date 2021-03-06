#pragma once

#include "tune.hpp"


class Param {
public:
    void set(Env const* e) {
        m_env = e;
        m_pos = 0;
    }
    float value() const { return m_val; }
    void tick() {
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
    int loop_count;
    int pos;
    int wait;
    int length;
    int note;
    int break_frame;

    // params
    std::array<Param, Inst::PARAM_COUNT_LOCAL> params;

    // param cache
    enum Wave { W_NOISE, W_PULSE, W_SAW, W_TRIANGLE, W_SINE };
    Wave  wave;
    float speed;
    float pulsewidth;
    float next_pulsewidth;
    float filter;
    float panning[2];
    float attack;
    float sustain;
    float decay;
    float release;


    // voice stuff
    enum State { S_RELEASE, S_ATTACK, S_HOLD };
    float    noise;
    uint32_t shift = 0x7ffff8;
    int      noise_phase;
    float    phase;
    State    state;
    float    level;
};

struct Filter {
    enum {
        T_LOW  = 1,
        T_BAND = 2,
        T_HIGH = 4,
    };
    int   type;
    float reso;
    float freq;

    float high[2];
    float band[2];
    float low[2];
};


class Synth {
public:
    Synth(Tune const& tune) : m_tune(tune) {}
    void init();
    void mix(int16_t* buffer, int len);
    bool tune_done() const;
private:
    void tick();

    int m_sample = 0;
    int m_frame  = 0;

    Filter                                      m_filter;
    std::array<Param, Inst::PARAM_COUNT_GLOBAL> m_params;

    Tune const&                                 m_tune;
    std::array<Channel, CHANNEL_COUNT>          m_channels;
};

