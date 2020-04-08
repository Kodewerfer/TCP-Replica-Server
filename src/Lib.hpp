#pragma once

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Lib
{
int readline(int, char *, size_t);
int readline(int, std::string &, size_t);
std::vector<char *> Tokenize(const std::string &str);
std::vector<std::string> TokenizeDeluxe(const std::string &str);
int recv_nonblock(int sd, char *buf, size_t max, int timeout);
}; // namespace Lib