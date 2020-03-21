#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../Utils.hpp"
#include "../lib.hpp"

struct ShellResponse {
    std::string status;
    int code;
    std::string message;
};

class ShellClient {
   private:
    static std::vector<std::string> FileNamePrefixs;
    std::string sLastOutput{"NO_LAST_COMMAND"};
    static std::string InterpMessage(ShellResponse &);

   public:
    ShellClient();
    std::string GetLastOutput() { return sLastOutput; };
    //
    ShellResponse RunShellCommand(std::vector<char *> &);
    ~ShellClient();
};