#pragma once

#include <string>

class ShellServer {
   private:
    /* data */
    static std::string sLastOutput;

   public:
    static std::string GetLastOutput() { return sLastOutput; };
    ShellServer(/* args */);
    ~ShellServer();
};