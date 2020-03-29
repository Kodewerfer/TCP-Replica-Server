#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace Lib {
int readline(int, char *, size_t);
int readline(int, std::string &, size_t);
std::vector<char *> Tokenize(const std::string &str);
};  // namespace lib