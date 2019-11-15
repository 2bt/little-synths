#pragma once

#include <vector>
#include <array>
#include <string>


enum {
    MIXRATE       = 44100,
    FRAME_LENGTH  = MIXRATE / 60,
    CHANNEL_COUNT = 4,
};


struct Env {
    struct Datum {
        bool  relative;
        float value;
    };
    std::vector<Datum> data;
    int                loop;
};


struct Inst {
    enum Param {
        P_ATTACK,
        P_DECAY,
        P_SUSTAIN,
        P_RELEASE,
        P_VOLUME,
        P_PANNING,
        P_WAVE,
        P_PULSEWIDTH,
        P_PITCH,
        P_BREAK,
        P_FILTER,
        //...
        PARAM_COUNT,

        // global params
        P_FILTER_TYPE,
        P_FILTER_FREQ,
        P_FILTER_RESO,
        P_TEMPO,
    };
    std::array<Env, PARAM_COUNT> params;
};


struct Track {
    struct Event {
        int wait;
        int note;
        int length;
        int inst_nr;
    };
    std::vector<Event> events;
};


struct Tune {
    std::vector<Inst>                insts;
    std::array<Track, CHANNEL_COUNT> tracks;
};

