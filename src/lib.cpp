#include "lib.h"

bool Lib::bRunningBackground{false};

int Lib::readline(int fd, char *buf_str, size_t max) {
    size_t i;
    for (i = 0; i < max; i++) {
        char tmp;
        int what = read(fd, &tmp, 1);
        if (what == 0 || tmp == '\n') {
            buf_str[i] = '\0';
            return i;
        }
        buf_str[i] = tmp;
    }
    buf_str[i] = '\0';
    return i;
}

int Lib::readline(int fd, std::string &sLine, size_t max) {
    size_t i;
    for (i = 0; i < max; i++) {
        char tmp;
        int what = read(fd, &tmp, 1);
        if (what == 0 || tmp == '\n') {
            // buf_str[i] = '\0';
            return i;
        }
        sLine += tmp;
    }
    return i;
}

std::vector<std::string> Lib::SplitBySpace(std::string str) {
    std::istringstream StreamStr(str);
    std::vector<std::string> VWords{};

    do {
        std::string Word;
        StreamStr >> Word;

        if (Word != "") VWords.push_back(Word);

    } while (StreamStr);

    return VWords;
}