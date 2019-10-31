#include "synth.hpp"


void Synth::mix(int16_t* buffer, int len) {
    for (int i = 0; i < len; ++i) {

        *buffer++ = clamp<int>(0 * 7000, -32768, 32767);
        *buffer++ = clamp<int>(0 * 7000, -32768, 32767);
    }
}

