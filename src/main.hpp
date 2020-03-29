#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "Utils.hpp"
#include "components/FileClient.hpp"
#include "components/STDResponse.hpp"
#include "components/ShellClient.hpp"
#include "lib.hpp"

struct ServerSockets {
    int shell;
    int file;
};

struct ServerPorts {
    int sh;
    int fi;
};

ServerSockets InitServer(int, int);
void PrintMessage(const int iSh, const int iFi);
void StartServer(ServerSockets ServSockets,
                 std::function<void(const int)> ShellCallback,
                 std::function<void(const int)> FileCallback);

void DoShellCallback(const int);
void DoFileCallback(const int);

ServerPorts ParsOpt(int, char **, const char *);
void daemonize();

// Signal handles
void HandleSIGs(int);