#include "main.hpp"
#define SHELL_DEFAULT 9000;
#define FILE_DEFAULT 9001;
int main(int arg, char *argv_main[], char *envp[]) {
    if (signal(SIGINT, HandleSIGs) == SIG_ERR)
        ServerUtils::buoy("can't catch SIGINT");

    ServerPorts FromOpts = ParsOpt(arg, argv_main, "f:s:dD");
    if (ServerUtils::bRunningBackground) {
        // daemon
        daemonize();
    }

    const int iShPort{FromOpts.sh};
    const int iFiPort{FromOpts.fi};

    ServerSockets ServSockets;
    try {
        ServSockets = InitServer(iShPort, iFiPort);
        PrintMessage(iShPort, iFiPort);
    } catch (char const *msg) {
        ServerUtils::buoy(msg);
    }

    StartServer(ServSockets, DoShellCallback, DoFileCallback);

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
    ServerUtils::buoy("Shell Server is listening on port " + std::to_string(iSh));
    ServerUtils::buoy("File Server is listening on port. " + std::to_string(iFi));
}

void StartServer(ServerSockets ServSockets,
                 std::function<void(const int)> ShellCallback,
                 std::function<void(const int)> FileCallback) {
    if (ServSockets.shell < 0 || ServSockets.file < 0) {
        ServerUtils::buoy("Server unable to start.");
        return;
    }

    sockaddr_in ClientAddrSTR;
    unsigned int iClientAddrLen = sizeof(ClientAddrSTR);

    std::thread Worker;
    // loop accepting
    while (true) {
        int iSockets[2]{ServSockets.shell, ServSockets.file};
        // the correct callback to call;
        std::function<void(const int)> TheCallback_PTR{nullptr};
        Accepted accepted_STR =
            ServerUtils::PollEither((int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
                              (socklen_t *)&iClientAddrLen);

        if (accepted_STR.accepted == ServSockets.shell) {
            // Shell server
            ServerUtils::buoy("Shell client incoming");
            TheCallback_PTR = ShellCallback;

        } else if (accepted_STR.accepted == ServSockets.file) {
            // File server
            ServerUtils::buoy("File client incoming");
            TheCallback_PTR = FileCallback;

        } else {
            ServerUtils::buoy("Failed to accept any client");
        }

        Worker = std::thread(TheCallback_PTR, accepted_STR.newsocket);
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
    bool bIsExceedLimt{!(ServerUtils::ShellServerLock.try_lock())};
    if (bIsExceedLimt) {
        WelcomeMessage =
            "Shell Server has exceeded its limit, terminating session...";
    }

    send(iServFD, "\n", 1, 0);
    send(iServFD, WelcomeMessage.c_str(), WelcomeMessage.size(), 0);
    send(iServFD, "\n", 1, 0);

    // Terminate the session if more than one user.
    if (bIsExceedLimt) {
        close(iServFD);
        shutdown(iServFD, 1);
        ServerUtils::buoy("Shell client Blocked");
        return;
    }

    // Normal proceeding.
    ShellClient *NewClient = new ShellClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    while ((lib::readline(iServFD, req, ALEN - 1)) != FLAG_NO_DATA) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = lib::Tokenize(sRequest);

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
    ServerUtils::buoy("Shell connection closed by client.");

    close(iServFD);
    shutdown(iServFD, 1);
    // Accepting new shell client.
    ServerUtils::ShellServerLock.unlock();
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

    while ((lib::readline(iServFD, req, ALEN - 1)) != FLAG_NO_DATA) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = lib::Tokenize(sRequest);
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
    ServerUtils::buoy("File Connection closed by client.");
    close(iServFD);
    shutdown(iServFD, 1);
}

/**
 *
 * Miscs
 *
 **/

ServerPorts ParsOpt(int argc, char **argv, const char *optstring) {
    int iShPort = SHELL_DEFAULT;
    int iFiPort = FILE_DEFAULT;

    int opt;
    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
            case 's': {  // shell port
                int temp = atoi(optarg);
                if (temp > 0) iShPort = temp;

                break;
            }
            case 'f': {
                int temp = atoi(optarg);
                if (temp > 0) iFiPort = temp;

                // file port
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

    return {iShPort, iFiPort};
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

void HandleSIGs(int sig) { std::cout << "SIGQUIT"; }