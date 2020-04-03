#include <arpa/inet.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

#include <future>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "Lib.hpp"
#include "ServerUtils.hpp"
#include "components/FileClient.hpp"
#include "components/STDResponse.hpp"
#include "components/ShellClient.hpp"
#include "threads/ThreadsMan.hpp"

struct OptParsed {
    const int sh;
    const int fi;
    const int tincr;
    const int tmax;
};

/**
 *  Main logic
 * */

ServerSockets InitServer(int, int);
void PrintMessage(const int iSh, const int iFi);
void CreateThreads(ServerSockets &ServSockets, OptParsed &,
                   std::function<void(const int)> ShellCallback,
                   std::function<void(const int)> FileCallback);

/**
 *  Callback for Shell or File server
 * */

void DoShellCallback(const int);
void DoFileCallback(const int);
// repilica handler
std::function<bool()> HandleSync(const std::string &request,
                                 const std::string ReqType);
/**
 *  Misc
 * */

OptParsed ParsOpt(int, char **, const char *);
void daemonize();

/**
 * Dynamic reconfiguration
 * */

void HandleSIGQUIT(int sig);
void HandleSIGHUP(int sig);