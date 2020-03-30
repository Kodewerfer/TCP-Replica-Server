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

#include "../Lib.hpp"
#include "../ServerUtils.hpp"

class ShellClient {
   private:
    /**
     * path prefix for execv
     */
    static std::vector<std::string> FileNamePrefixs;
    /**
     * Store the last execv output
     */
    std::string sLastOutput;
    void ResetLastOutput() { sLastOutput = ""; }

   public:
    ShellClient();
    std::string &GetLastOutput() { return sLastOutput; };
    /**
     * Run the command
     */
    int RunShellCommand(std::vector<char *> &);
};