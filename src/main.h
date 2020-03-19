#include <signal.h>

#include <string>
#include <vector>

#include "Utils.h"
#include "lib.h"

struct ServerSockets {
    int shell;
    int file;
};

ServerSockets InitServer();
void StartServer(ServerSockets ServSockets);
void PrintMessage(const int iSh, const int iFi);
void DoShell(const int iServFD);
void DoFile(const int iServFD);
void SigPipeHandle(int signum);
