#include "parser.hpp"
#include "synth.hpp"
#include <fstream>
#include <iostream>
#include <SDL2/SDL.h>
#include <sndfile.h>



Tune tune;
Synth synth(tune);

int main(int argc, const char** argv) {
    bool usage           = false;
    bool write           = false;
    if (argc == 3) {
        usage = argv[1] != std::string("-w");
        write = true;
    }
    else usage = argc != 2;
    if (usage) {
        printf("usage: %s [-w] songfile\n", argv[0]);
        return 0;
    }

    std::ifstream file(argv[argc - 1]);
    if (!file.is_open()) {
        printf("error: cannot open file\n");
        return 1;
    }
    std::string content = {
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    };

    Parser parser(content.c_str());
    parser.parse_tune(tune);

    synth.init();

    if (write) {
        SF_INFO info = { 0, MIXRATE, 2, SF_FORMAT_WAV | SF_FORMAT_PCM_16 };
        SNDFILE* f = sf_open("out.wav", SFM_WRITE, &info);
        int16_t buffer[FRAME_LENGTH * 2];
        while (!synth.tune_done()) {
            synth.mix(buffer, FRAME_LENGTH);
            sf_writef_short(f, buffer, FRAME_LENGTH);
        }
        sf_close(f);
        return 0;
    }


    SDL_AudioSpec spec = {
        MIXRATE, AUDIO_S16, 2, 0, 1024, 0, 0,
        [](void* u, Uint8* stream, int len) { synth.mix((short*) stream, len / 4); }
    };

    SDL_OpenAudio(&spec, nullptr);
    SDL_PauseAudio(0);
    printf("playing...\n");
    getchar();
    printf("done.\n");
    SDL_Quit();

    return 0;
}
