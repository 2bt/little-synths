#include "parser.hpp"
#include <cstring>

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
    while (isalnum(chr()));
    return name;
}


Env Parser::parse_env() {
    // alias
    if (chr() == '@') {
        next_chr();
        std::string name = parse_name();
        skip_space();
        auto it = m_envs.find(name);
        if (it == m_envs.end()) {
            printf("%d: error: invalid env reference '%s'\n", m_line, name.c_str());
            exit(1);
        }
        return it->second;
    }

    Env env;
    env.loop = -1;
    for (;;) {
        // loop point
        if (chr() == '|') {
            next_chr();
            skip_space();
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
        skip_space();

        if (relative && number.empty()) {
            printf("%d: error: invalid env\n", m_line);
            exit(1);
        }
        if (number.empty()) break;

        env.data.push_back({ relative, std::stof(number, nullptr) });
    }
    if (env.loop == -1) env.loop = env.data.size() - 1;
    return env;
}


Inst Parser::parse_inst() {
    Inst inst;
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
    }

    consume('{');
    skip_space(true);
    while (chr() != '}') {
        std::string name = parse_name();
        skip_space();
        consume('=');
        skip_space();

        static const std::map<std::string, Inst::Param> name_map = {
            { "attack",     Inst::P_ATTACK     },
            { "decay",      Inst::P_DECAY      },
            { "sustain",    Inst::P_SUSTAIN    },
            { "release",    Inst::P_RELEASE    },
            { "gate",       Inst::P_GATE       },
            { "volume",     Inst::P_VOLUME     },
            { "panning",    Inst::P_PANNING    },
            { "waveform",   Inst::P_WAVEFORM   },
            { "pulsewidth", Inst::P_PULSEWIDTH },
            { "pitch",      Inst::P_PITCH      },
        };

        auto it = name_map.find(name);
        if (it == name_map.end()) {
            printf("%d: error: invalid inst param '%s'\n", m_line, name.c_str());
            exit(1);
        }
        inst.params[it->second] = parse_env();

        if (chr() == '}') break;
        if (chr() == '\n') next_chr();
        else consume(';');
        skip_space(true);
    }
    next_chr();

    return inst;
}


Tune Parser::parse_tune() {
    Tune tune;
    while (chr()) {
        skip_space(true);
        char c = next_chr();
        if (c == '@') {
            std::string name = parse_name();
            skip_space();
            consume('=');
            skip_space();
            auto p = m_envs.insert({ name, parse_env() });
            if (!p.second) {
                printf("%d: error: env name '%s' already assigned\n", m_line, name.c_str());
                exit(1);
            }
            if (chr() == '\n') next_chr();
            else consume(';');

            // print env
            printf("@%s =", name.c_str());
            int i = 0;
            Env const& e = m_envs[name];
            for (auto d : e.data) printf(" %s%s%g", i++ == e.loop ? "| " : "", "+" + !d.relative, d.value);
            printf("\n");
        }
        else if (c == '$') {
            std::string name = parse_name();
            skip_space();
            consume('=');
            skip_space();
            auto p = m_insts.insert({ name, parse_inst() });
            if (!p.second) {
                printf("%d: error: inst name '%s' already assigned\n", m_line, name.c_str());
                exit(1);
            }
            if (chr() == '\n') next_chr();
            else consume(';');
        }
        else {
            printf("%d: error: invalid char '%c'\n", m_line, c);
            exit(1);
        }
    }
    return tune;
}

