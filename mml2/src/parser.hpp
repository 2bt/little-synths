#pragma once

#include "tune.hpp"
#include <map>


class Parser {
public:
    Parser(char const* src) : m_pos(src) {}

    void parse_tune(Tune& tune);

private:
    char chr() const { return *m_pos; }
    char next_chr() {
        char c = *m_pos;
        if (c != '\0') ++m_pos;
        if (c == '\n') ++m_line;
        return c;
    }

    void consume(char c);
    void skip_space(bool newline=false);

    std::string parse_name();
    uint32_t    parse_uint();
    Env         parse_env();
    void        parse_inst(Inst& inst);
    void        parse_track(Tune& tune, int nr);

    char const*                 m_pos;
    int                         m_line = 1;
    std::map<std::string, Env>  m_envs;
    std::map<std::string, Inst> m_insts;
};


