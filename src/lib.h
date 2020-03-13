#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sstream>

class Lib {
   private:
    static bool bRunningBackground;

   public:
    static void SetBackgroundRunning(bool);
    static int readline(int, char *, size_t);
    static int readline(int, std::string &, size_t);
    static std::vector<std::string> SplitBySpace(std::string);
};  // namespace Lib