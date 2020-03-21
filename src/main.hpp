#include <signal.h>

#include <functional>
#include <string>
#include <vector>

#include "Utils.hpp"
#include "components/ShellClient.hpp"
#include "lib.hpp"

struct ServerSockets {
    int shell;
    int file;
};

ServerSockets InitServer();
void StartServer(ServerSockets ServSockets,
                 std::function<void(const int)> ShellCallback,
                 std::function<void(const int)> FileCallback);
void PrintMessage(const int iSh, const int iFi);

void DoFileCallback(const int);
void DoShellCallback(const int);

void SigPipeHandle(int signum);