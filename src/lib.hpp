#pragma once

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Lib {
void SetBackgroundRunning(bool);
int readline(int, char *, size_t);
int readline(int, std::string &, size_t);
std::vector<std::string> SplitBySpace(std::string);
};  // namespace Lib