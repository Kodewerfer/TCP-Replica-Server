#include "lib.hpp"

namespace Lib {
int readline(int fd, char *buf_str, size_t max) {
    size_t i;
    int begin = 1;

    for (i = 0; i < max; i++) {
        char tmp;
        int what = read(fd, &tmp, 1);
        if (what == -1) return -1;
        if (begin) {
            if (what == 0) return -2;  // no more data
            begin = 0;
        }
        if (what == 0 || tmp == '\n') {
            buf_str[i] = '\0';
            return i;
        }
        buf_str[i] = tmp;
    }
    buf_str[i] = '\0';
    return i;
}

int readline(int fd, std::string &sLine, size_t max) {
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

std::vector<std::string> SplitBySpace(std::string str) {
    std::istringstream StreamStr(str);
    std::vector<std::string> VWords{};

    do {
        std::string Word;
        StreamStr >> Word;

        if (Word != "") VWords.push_back(Word);

    } while (StreamStr);

    return VWords;
}
}  // namespace Lib
