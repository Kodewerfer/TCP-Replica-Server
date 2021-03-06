#include "Lib.hpp"

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

int recv_nonblock(int sd, char *buf, size_t max, int timeout) {
    pollfd pollrec;
    pollrec.fd = sd;
    pollrec.events = POLLIN;

    int polled = poll(&pollrec, 1, timeout);

    if (polled == 0) return -2;
    if (polled == -1) return -1;

    return recv(sd, buf, max, 0);
}

std::vector<char *> Tokenize(const std::string &sUserInput) {
    std::istringstream StreamStr(sUserInput);
    std::vector<char *> VWords;

    do {
        std::string Word;
        StreamStr >> Word;
        char *cstr = (char *)Word.c_str();
        char *temp = new char(sizeof(cstr));
        strcpy(temp, cstr);

        if (Word != "") VWords.push_back(temp);

    } while (StreamStr);

    return VWords;
}

std::vector<std::string> TokenizeDeluxe(const std::string &sUserInput) {
    std::istringstream StreamStr(sUserInput);
    std::vector<std::string> VWords;

    do {
        std::string Word;
        StreamStr >> Word;

        if (Word != "") VWords.push_back(Word);

    } while (StreamStr);

    return VWords;
}
}  // namespace Lib
