#pragma once

#include <vector>
#include <array>
#include <string>


enum {
    MIXRATE       = 44100,
    FRAME_LENGTH  = MIXRATE / 50,
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
    enum {
        L_GATE,
        L_ATTACK,
        L_DECAY,
        L_SUSTAIN,
        L_RELEASE,
        L_VOLUME,
        L_PANNING,
        L_WAVE,
        L_PULSEWIDTH,
        L_PITCH,
        L_BREAK,
        L_FILTER,
        //...
        PARAM_COUNT_LOCAL,

        G_FILTER_TYPE = 0,
        G_FILTER_FREQ,
        G_FILTER_RESO,
        G_TEMPO,
        // ...
        PARAM_COUNT_GLOBAL,
        PARAM_COUNT_TOTAL = PARAM_COUNT_LOCAL + PARAM_COUNT_GLOBAL,
    };
    std::array<Env, PARAM_COUNT_TOTAL> envs;
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

