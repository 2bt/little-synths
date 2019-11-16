#include "parser.hpp"
#include <cstring>
#include <algorithm>


namespace {

int find(const char* str, char c) {
    if (c == '\0') return -1;
    char const* p = strchr(str, c);
    return p ? p - str : -1;
}

} // namespace


bool operator==(Env::Datum const& a, Env::Datum const& b) {
    return a.relative == b.relative && a.value == b.value;
}
bool operator==(Env const& a, Env const& b) {
    return a.loop == b.loop && a.data == b.data;
}
bool operator==(Inst const& a, Inst const& b) {
    return a.envs == b.envs;
}


void Parser::consume(char c) {
    if (chr() != c) {
        printf("%d: error: read '%c' but expected '%c'\n", m_line, chr(), c);
        exit(1);
    }
    next_chr();
}

void Parser::skip_space(bool newline) {
    for (;;) {
        while (isspace(chr()) && (newline || chr() != '\n')) next_chr();
        if (chr() != '#') break;
        while (chr() != '\0' && chr() != '\n') next_chr();
        if (newline && chr() == '\n') next_chr();
    }
}

std::string Parser::parse_name() {
    if (!isalpha(chr())) {
        printf("%d: error: read '%c' but expected letter\n", m_line, chr());
        exit(1);
    }
    std::string name;
    do name += next_chr();
    while (isalnum(chr()) || chr() == '-');
    return name;
}

uint32_t Parser::parse_uint() {
    if (!isdigit(chr())) {
        printf("%d: error: read '%c' but expected digit\n", m_line, chr());
        exit(1);
    }
    uint32_t n = 0;
    do n = n * 10 + next_chr() - '0';
    while (isdigit(chr()));
    return n;
}

Env Parser::parse_env() {
    Env env;
    env.loop = -1;
    std::vector<int> repeat;
    for (;;) {
        skip_space();

        if (chr() == '[') {
            next_chr();
            repeat.push_back(env.data.size());
            continue;
        }
        if (chr() == ']') {
            if (repeat.empty()) {
                printf("%d: error: invalid env\n", m_line);
                exit(1);
            }
            next_chr();
            skip_space();
            int n = parse_uint();
            int x = repeat.back();
            repeat.pop_back();
            int y = env.data.size();
            while (--n > 0) {
                env.data.insert(env.data.end(), env.data.begin() + x, env.data.begin() + y);
            }
            continue;
        }

        // loop point
        if (chr() == '|') {
            next_chr();
            skip_space();
            if (env.loop != -1) {
                printf("%d: error: invalid env: multiple loop points\n", m_line);
                exit(1);
            }
            env.loop = env.data.size();
        }

        bool relative = chr() == '+';
        if (relative) {
            next_chr();
            skip_space();
        }

        std::string number;
        if (chr() == '-') number += next_chr();
        while (isdigit(chr())) number += next_chr();
        if (chr() == '.') {
            number += next_chr();
            while (isdigit(chr())) number += next_chr();
        }

        if (relative && number.empty()) {
            printf("%d: error: invalid env\n", m_line);
            exit(1);
        }
        if (number.empty()) break;

        env.data.push_back({ relative, std::stof(number, nullptr) });
    }
    skip_space();
    if (env.loop == -1) env.loop = env.data.size() - 1;
    if (!repeat.empty()) {
        printf("%d: error: invalid env\n", m_line);
        exit(1);
    }
    return env;
}

void Parser::parse_inst(Inst& inst) {
    if (chr() == '$') {
        next_chr();
        std::string name = parse_name();
        skip_space();
        auto it = m_insts.find(name);
        if (it == m_insts.end()) {
            printf("%d: error: invalid inst reference '%s'\n", m_line, name.c_str());
            exit(1);
        }
        inst = it->second;
        if (chr() != '{') return;
    }

    consume('{');
    skip_space(true);
    while (chr() != '}') {
        std::string name = parse_name();
        skip_space();
        consume('=');

        static const std::array<std::string, Inst::PARAM_COUNT_TOTAL> names = {
            "attack",
            "decay",
            "sustain",
            "release",
            "volume",
            "panning",
            "wave",
            "pulsewidth",
            "pitch",
            "break",
            "filter",
            // ---
            "filter-type",
            "filter-freq",
            "filter-reso",
            "tempo",
        };
        int i;
        for (i = 0; i < (int) names.size(); ++i) {
            if (names[i].find(name) == 0)  break;
        }
        if (i == names.size()) {
            printf("%d: error: invalid inst param '%s'\n", m_line, name.c_str());
            exit(1);
        }
        inst.envs[i] = parse_env();

        if (chr() == '}') break;
        if (chr() == '\n') next_chr();
        else consume(';');
        skip_space(true);
    }
    next_chr();
}

void Parser::parse_track(Tune& tune, int nr) {
    Track& track = tune.tracks[nr];
    int   octave = 4;
    int   wait   = 4;
    int   length = 0;
    Inst  inst;


    std::vector<int> repeat;
    for (;;) {
        skip_space();

        if (chr() == '[') {
            next_chr();
            repeat.push_back(track.events.size());
            continue;
        }
        if (chr() == ']') {
            if (repeat.empty()) {
                printf("%d: error: invalid env\n", m_line);
                exit(1);
            }
            next_chr();
            skip_space();
            int n = parse_uint();
            int x = repeat.back();
            repeat.pop_back();
            int y = track.events.size();
            while (--n > 0) {
                track.events.insert(track.events.end(), track.events.begin() + x, track.events.begin() + y);
            }
            continue;
        }

        // length
        if (chr() == 'l') {
            next_chr();
            length = parse_uint();
            continue;
        }
        // octave
        if (chr() == 'o') {
            next_chr();
            octave = parse_uint();
            continue;
        }
        if (chr() == '>' || chr() == '<') {
            octave += next_chr() == '>' ? 1 : -1;
            continue;
        }


        // instrument
        if (chr() == '$' || chr() == '{') {
            parse_inst(inst);
            continue;
        }

        // note or rest
        int index = find("ccddeffggaabr", chr());
        if (index >= 0) {
            next_chr();
            int note = -1;
            if (index < 12) {
                note = index + 12 * octave;
                while (chr() == '+' || chr() == '-') note += next_chr() == '+' ? 1 : -1;
            }
            if (isdigit(chr())) wait = parse_uint();

            // instrument
            if (chr() == '$' || chr() == '{') parse_inst(inst);

            int inst_nr = tune.insts.size();
            auto it = std::find(tune.insts.begin(), tune.insts.end(), inst);
            if (it != tune.insts.end()) inst_nr = it - tune.insts.begin();
            else tune.insts.push_back(inst);

            track.events.push_back({ wait, note, length > 0 ? length : wait, inst_nr });
            continue;
        }
        return;
    }
}

void Parser::parse_tune(Tune& tune) {
    skip_space(true);
    while (chr()) {
        if (chr() == '$') {
            next_chr();
            std::string name = parse_name();
            skip_space();
            consume('=');
            skip_space();
            Inst inst;

            // set defaults
            inst.envs[Inst::L_VOLUME]  = Env{ { { false,   1 } }, 0 };
            inst.envs[Inst::L_ATTACK]  = Env{ { { false,   2 } }, 0 };
            inst.envs[Inst::L_DECAY]   = Env{ { { false, 200 } }, 0 };
            inst.envs[Inst::L_RELEASE] = Env{ { { false, 100 } }, 0 };
            inst.envs[Inst::L_SUSTAIN] = Env{ { { false, 0.7 } }, 0 };

            parse_inst(inst);
            auto p = m_insts.insert({ name, std::move(inst) });
            if (!p.second) {
                printf("%d: error: inst name '%s' already assigned\n", m_line, name.c_str());
                exit(1);
            }
        }
        else if (isdigit(chr())) {
            int nr = parse_uint();
            if (nr >= CHANNEL_COUNT) {
                printf("%d: error: track number too high\n", m_line);
                exit(1);
            }
            skip_space();
            consume(':');
            skip_space();
            parse_track(tune, nr);
        }
        else {
            printf("%d: error: invalid char '%c'\n", m_line, chr());
            exit(1);
        }
        if (chr() == '\n') next_chr();
        else consume(';');
        skip_space(true);
    }
}
