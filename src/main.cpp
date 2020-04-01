#include "main.hpp"

#define SHELL_DEFAULT 9000;
#define FILE_DEFAULT 9001;
#define THREAD_INCR_DEFAULT 2;
#define THREAD_MAX_DEFAULT 4;

int main(int arg, char *argv_main[], char *envp[]) {
    OptParsed FromOpts = ParsOpt(arg, argv_main, "f:s:T:t:dD");
    // For debugging output.
    ThreadsMan::T_incr = FromOpts.tincr;

    // daemonize
    if (ServerUtils::bRunningBackground) {
        daemonize();
    }

    ServerSockets ServSockets;
    try {
        const int iShPort{FromOpts.sh};
        const int iFiPort{FromOpts.fi};
        ServSockets = InitServer(iShPort, iFiPort);
        PrintMessage(iShPort, iFiPort);
        // store a reference.
        ServerUtils::setSocketsRef(ServSockets);
    } catch (char const *msg) {
        ServerUtils::buoy(msg);
    }

    /**
     * Dynamic reconfiguration
     * */

    if (signal(SIGQUIT, HandleSIGQUIT) == SIG_ERR)
        ServerUtils::buoy("Can't catch SIGQUIT");
    if (signal(SIGHUP, HandleSIGHUP) == SIG_ERR)
        ServerUtils::buoy("Can't catch SIGHUP");

    /**
     *   Seperated Accepting and Creating logics.
     *   Main thread handle creation while threads themselves handle accepting
     *   new client.
     *  */
    std::mutex LoopLock;
    std::unique_lock<std::mutex> Locker(LoopLock);
    // wait on condition varible notification.
    while (true) {
        ThreadsMan::NeedMoreThreads.wait(Locker, [FromOpts]() {
            // lambda "guard" for the condition.
            const int MaxThread = FromOpts.tincr + FromOpts.tmax;
            bool bAllThreadsActive =
                ThreadsMan::getThreadsCount() == ThreadsMan::getActiveThreads();
            bool bLimitNotReached = ThreadsMan::getThreadsCount() < MaxThread;

            bool bSigHup = ServerUtils::testSighup();

            return (bSigHup || (bAllThreadsActive && bLimitNotReached));
        });

        ServerUtils::rowdy("Main thread awaken, creating threads...");

        CreateThreads(ServSockets, FromOpts, DoShellCallback, DoFileCallback);

        ServerUtils::rowdy("Threads Count now : " +
                           std::to_string(ThreadsMan::getThreadsCount()));
        ServerUtils::rowdy("Threads Active now : " +
                           std::to_string(ThreadsMan::getActiveThreads()));
    }

    return EXIT_SUCCESS;
}

/**
 *
 * Main logics
 *
 **/
ServerSockets InitServer(int iSh, int iFi) {
    // init shell server.
    int iShServSocket{-1};
    int iFiServSocket{-1};

    iShServSocket = ServerUtils::CreateSocketMasterLocalOnly(iSh);
    iFiServSocket = ServerUtils::CreateSocketMaster(iFi);

    return {iShServSocket, iFiServSocket};
}

void PrintMessage(const int iSh, const int iFi) {
    ServerUtils::buoy("SHFD Shell and file server.");
    ServerUtils::buoy("Shell Server is listening on port " +
                      std::to_string(iSh));
    ServerUtils::buoy("File Server is listening on port. " +
                      std::to_string(iFi));
}

void CreateThreads(ServerSockets &ServSockets, OptParsed &FromOpts,
                   std::function<void(const int)> ShellCallback,
                   std::function<void(const int)> FileCallback) {
    int i = 1;
    while (i <= FromOpts.tincr) {
        std::thread Worker(ThreadsMan::ForeRunner, ServSockets, ShellCallback,
                           FileCallback);

        // ThreadsMan::ThreadStash.push_back(move(Worker));
        Worker.detach();
        ThreadsMan::ThreadCreated();

        i++;
    }
}

/**
 *
 * Callback functions
 *
 **/
void DoShellCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];
    // send welcome message
    std::string WelcomeMessage = "Shell Server Connected.";

    // NEW - Shell client limit
    // !! LOCKING !!
    // !! LOCKING !!
    std::unique_lock<std::mutex> ShellLock(ServerUtils::ShellServerLock,
                                           std::try_to_lock);
    if (!ShellLock.owns_lock()) {
        WelcomeMessage =
            "Shell Server has exceeded its limit, terminating session...";
    }

    send(iServFD, "\n", 1, 0);
    send(iServFD, WelcomeMessage.c_str(), WelcomeMessage.size(), 0);
    send(iServFD, "\n", 1, 0);

    // Terminate the session if more than one user.
    if (!ShellLock.owns_lock()) {
        close(iServFD);
        shutdown(iServFD, 1);
        ServerUtils::buoy("Shell client Blocked");
        return;
    }

    // Normal proceeding.
    ShellClient *NewClient = new ShellClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    int n{0};
    while (n = (Lib::readline(iServFD, req, ALEN - 1)) >= 0) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = Lib::Tokenize(sRequest);

        if (strcmp(RequestTokenized.at(0), "CPRINT") == 0) {
            std::string &LastOutput = NewClient->GetLastOutput();
            if (LastOutput == "MARK-NO-COMMAND") {
                NewRes->shell(-3);
                continue;
            }
            send(iServFD, LastOutput.c_str(), LastOutput.size(), 0);
            NewRes->shell(0);
            continue;
        }
        int ShellRes;
        try {
            ShellRes = NewClient->RunShellCommand(RequestTokenized);
            NewRes->shell(ShellRes);
        } catch (const std::string &e) {
            NewRes->fail(e);
            ServerUtils::buoy(e);
        }
    }

    if (n = -2) {
        ServerUtils::buoy("Shell Connection closed by client.");
    } else {
        ServerUtils::buoy("Server Quited.");
    }
}

void DoFileCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];
    // send welcome message
    const char welcome[] = "File Server Connected.";
    send(iServFD, "\n", 1, 0);
    send(iServFD, welcome, strlen(welcome), 0);
    send(iServFD, "\n", 1, 0);

    FileClient *NewClient = new FileClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    int n{0};
    while (n = (Lib::readline(iServFD, req, ALEN - 1)) >= 0) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = Lib::Tokenize(sRequest);
        char *TheCommand{RequestTokenized.at(0)};

        int res{-5};
        std::string message{" "};
        try {
            // FOPEN
            if (strcmp(TheCommand, "FOPEN") == 0) {
                int usedFd{-1};
                res = NewClient->FOPEN(RequestTokenized, usedFd);
                if (usedFd > 0) {
                    NewRes->fileFdInUse(usedFd);
                }
            }
            // FSEEK
            if (strcmp(TheCommand, "FSEEK") == 0) {
                res = NewClient->FSEEK(RequestTokenized);
            }
            // FREAD
            if (strcmp(TheCommand, "FREAD") == 0) {
                res = NewClient->FREAD(RequestTokenized, message);
            }
            // FWRITE
            if (strcmp(TheCommand, "FWRITE") == 0) {
                res = NewClient->FWRITE(RequestTokenized);
            }
            // FCLOSE
            if (strcmp(TheCommand, "FCLOSE") == 0) {
                res = NewClient->FCLOSE(RequestTokenized);
            }
        } catch (const std::string &e) {
            NewRes->fail(e);
            continue;
        }

        NewRes->file(res, message);
    }
    if (n = -2) {
        ServerUtils::buoy("File Connection closed by client.");
    } else {
        ServerUtils::buoy("Server Quited.");
    }
}

/**
 *
 * Miscs
 *
 **/

OptParsed ParsOpt(int argc, char **argv, const char *optstring) {
    int iShPort = SHELL_DEFAULT;
    int iFiPort = FILE_DEFAULT;
    int iThreadIncr = THREAD_INCR_DEFAULT;
    int iThreadMax = THREAD_MAX_DEFAULT;

    int opt;
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 's': {
                // shell port
                int temp = atoi(optarg);
                if (temp > 0) iShPort = temp;

                break;
            }
            case 'f': {
                // file port
                int temp = atoi(optarg);
                if (temp > 0) iFiPort = temp;

                break;
            }
            case 't': {
                // t incr
                int temp = atoi(optarg);
                if (temp > 0) iThreadIncr = temp;

                break;
            }
            case 'T': {
                // t max
                int temp = atoi(optarg);
                if (temp > 0) iThreadMax = temp;

                break;
            }
            case 'd': {
                // front run
                ServerUtils::bRunningBackground = false;
                break;
            }
            case 'D': {
                // debug
                ServerUtils::bIsDebugging = true;
                FileClient::bIsDebugging = true;
                break;
            }
        }
    }

    return {iShPort, iFiPort, iThreadIncr, iThreadMax};
}
void daemonize() {
    pid_t pid;

    /* Fork off the parent process */
    pid = fork();

    if (pid < 0) exit(EXIT_FAILURE);

    if (pid > 0) exit(EXIT_SUCCESS);

    if (setsid() < 0) exit(EXIT_FAILURE);

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    if (pid < 0) exit(EXIT_FAILURE);

    if (pid > 0) exit(EXIT_SUCCESS);

    // file permission
    umask(0);

    /* Close all open file descriptors */
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    return;
}

void HandleSIGQUIT(int sig) {
    ServerUtils::rowdy("SIGQUIT ");
    // set the flag
    ThreadsMan::StartQuiting();
    // close master sockets.
    std::array<int, 2> Socks = ServerUtils::getSocketsRef();
    for (auto i : Socks) {
        // close(i);
        shutdown(i, SHUT_RD);
        close(i);
    }

    // Kill all on going connections.
    ThreadsMan::closeAllSSocks();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // QUIT
    _Exit(EXIT_SUCCESS);
}

void HandleSIGHUP(int sig) {
    ServerUtils::rowdy("SIGHUP ");
    ServerUtils::SigHupReconfig();
    ThreadsMan::NeedMoreThreads.notify_one();
    return;
};