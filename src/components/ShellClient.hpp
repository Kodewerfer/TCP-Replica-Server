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

#include "../ServerUtils.hpp"
#include "../lib.hpp"

class ShellClient {
   private:
    static std::vector<std::string> FileNamePrefixs;
    std::string sLastOutput;
    void ResetLastOutput() { sLastOutput = ""; }

   public:
    ShellClient();
    std::string &GetLastOutput() { return sLastOutput; };
    //
    int RunShellCommand(std::vector<char *> &);
};