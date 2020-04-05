#include "main.hpp"

#define SHELL_DEFAULT 9000;
#define FILE_DEFAULT 9001;
#define THREAD_INCR_DEFAULT 2;
#define THREAD_MAX_DEFAULT 4;

int main(int arg, char *argv_main[], char *envp[]) {
    OptParsed FromOpts = ParsOpt(arg, argv_main, "f:s:T:t:p:dDv");
    // remember important params
    ThreadsMan::T_incr = FromOpts.tincr;
    ServerUtils::PortsReference = {FromOpts.sh, FromOpts.fi};

    // daemonize
    if (ServerUtils::bRunningBackground) {
        daemonize();
    }

    // re-usable
    StartServer();

    /**
     * Dynamic reconfiguration
     * */
    signal(SIGPIPE, SIG_IGN);
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

            bool bSigHup = ServerUtils::trySighupFlag();

            return (bSigHup || (bAllThreadsActive && bLimitNotReached));
        });

        ServerUtils::rowdy("Main thread awaken, creating threads...");

        CreateThreads(ServerUtils::getSocketsRef(), FromOpts, DoShellCallback,
                      DoFileCallback);

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

void StartServer() {
    ServerSockets ServSockets;
    try {
        const int iShPort{ServerUtils::PortsReference.shell};
        const int iFiPort{ServerUtils::PortsReference.file};
        ServSockets = InitSockets(iShPort, iFiPort);
        PrintMessage(iShPort, iFiPort);
        // store a reference.
        ServerUtils::setSocketsRef(ServSockets);
    } catch (char const *msg) {
        ServerUtils::buoy(msg);
    }
}

ServerSockets InitSockets(int iSh, int iFi) {
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

    if (ServerUtils::PeersAddr.size() > 0) {
        ServerUtils::buoy(std::to_string(ServerUtils::PeersAddr.size()) +
                          " Peer(s) found. Running in Replica front-end mode.");
    }
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
    std::string WelcomeMsg = "\nShell Server Connected.\n";

    // NEW - Shell client limit
    // !! LOCKING !!
    std::unique_lock<std::mutex> ShellLock(ServerUtils::ShellServerLock);
    // if (!ShellLock.owns_lock()) {
    //     WelcomeMsg =
    //         "Shell Server has exceeded its limit, terminating session...";
    // }

    send(iServFD, WelcomeMsg.c_str(), WelcomeMsg.size(), 0);

    // Terminate the session if more than one user.
    // if (!ShellLock.owns_lock()) {
    //     close(iServFD);
    //     shutdown(iServFD, 1);
    //     ServerUtils::buoy("Shell client Blocked");
    //     return;
    // }

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
            if (ShellRes == 254) {
                throw std::string("ERSHL");
            }
            NewRes->shell(ShellRes);
        } catch (const std::string &e) {
            NewRes->fail(e);
            ServerUtils::buoy(e);
        }
    }  // core client loop ends

    ServerUtils::buoy("Connection closed: Shell Client.");

    delete NewClient;
    delete NewRes;
}

void DoFileCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];

    std::string WelcomeMsg{"\nFile Server Connected. \n"};

    std::vector<SyncPoint> PeersStash;

    FileClient *NewClient = new FileClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    // if the server is syncing with other.
    const bool bSendSyncRequests{ServerUtils::PeersAddr.size() > 0};

    // establish peer links
    int iDeadPeers{0};
    if (bSendSyncRequests) {
        WelcomeMsg += "Running In Replica mode. \n";

        for (auto peer : ServerUtils::PeersAddr) {
            SyncPoint SPoint;
            try {
                SPoint.init(peer);
            } catch (const std::string &e) {
                SPoint.isDead();
                ++iDeadPeers;
            }
            // save it to the stash for use in handleSync.
            PeersStash.push_back(SPoint);
        }
    }
    if (bSendSyncRequests && iDeadPeers > 0) {
        WelcomeMsg += "\nBe Advised: Cannot Connect to " +
                      std::to_string(iDeadPeers) + " Peer(s)\n";
        WelcomeMsg += "Actions Will Not Be In-Sync\n";
        ServerUtils::rowdy(std::to_string(iDeadPeers) + " Peer(s) is dead");
    }

    // send welcome messages.
    send(iServFD, WelcomeMsg.c_str(), WelcomeMsg.size(), 0);

    // Main Loop
    int n{0};
    while (n = (Lib::readline(iServFD, req, ALEN - 1)) >= 0) {
        const std::string sRequest(req);

        std::vector<char *> RequestTokenized = Lib::Tokenize(sRequest);
        char *TheCommand{RequestTokenized.at(0)};

        // Important parameters
        int iOPResult{NO_VALID_COM};
        std::string ResponseMessage{" "};
        std::function<bool(const int &, const std::string &)> SyncCallback;
        //
        try {
            /**
             * Local Operations
             * */
            // FOPEN
            int iPreviousFd{-1};
            if (strcmp(TheCommand, "FOPEN") == 0) {
                iOPResult = NewClient->FOPEN(RequestTokenized, iPreviousFd);
            }
            // FSEEK
            if (strcmp(TheCommand, "FSEEK") == 0) {
                iOPResult = NewClient->FSEEK(RequestTokenized);
                // sync wouldn't run if local request reported error
                if (bSendSyncRequests && iOPResult >= 0) {
                    // Build the sync request
                    std::string SyncRequest =
                        NewClient->SyncRequestBuilder(RequestTokenized);
                    // send the sync requests.
                    SyncCallback =
                        HandleSync(PeersStash, SyncRequest, "SYNCSEEK");
                }
            }
            // FREAD
            if (strcmp(TheCommand, "FREAD") == 0) {
                // read request return file content in an out param.
                iOPResult = NewClient->FREAD(RequestTokenized, ResponseMessage);
                // sync wouldn't run if local request reported error
                if (bSendSyncRequests && iOPResult >= 0) {
                    // Build the sync request
                    std::string SyncRequest =
                        NewClient->SyncRequestBuilder(RequestTokenized);
                    // send the sync requests.
                    SyncCallback =
                        HandleSync(PeersStash, SyncRequest, "SYNCREAD");
                }
            }
            // FWRITE
            if (strcmp(TheCommand, "FWRITE") == 0) {
                // local request
                iOPResult = NewClient->FWRITE(RequestTokenized);
                // sync wouldn't run if local request reported error
                if (bSendSyncRequests && iOPResult >= 0) {
                    // Build the sync request
                    std::string SyncRequest =
                        NewClient->SyncRequestBuilder(RequestTokenized);
                    // send the sync requests.
                    SyncCallback =
                        HandleSync(PeersStash, SyncRequest, "SYNCWRITE");
                }
            }
            // FCLOSE
            if (strcmp(TheCommand, "FCLOSE") == 0) {
                iOPResult = NewClient->FCLOSE(RequestTokenized);
            }

            /**
             * Handle Sync Requests
             * Server to Server
             * */
            if (strcmp(TheCommand, "SYNCWRITE") == 0) {
                iOPResult = NewClient->SYNCWRITE(RequestTokenized);
            }
            if (strcmp(TheCommand, "SYNCREAD") == 0) {
                iOPResult =
                    NewClient->SYNCREAD(RequestTokenized, ResponseMessage);
            }
            if (strcmp(TheCommand, "SYNCSEEK") == 0) {
                iOPResult = NewClient->SYNCSEEK(RequestTokenized);
            }

            /**
             * Send the response to user
             * */
            if (bSendSyncRequests && SyncCallback != nullptr) {
                // origin of the sync requests.
                // pass on the result from local operation.
                if (SyncCallback(iOPResult, ResponseMessage)) {
                    NewRes->file(iOPResult, ResponseMessage);
                } else {
                    throw std::string("ER-SYNC");
                }
            } else if (iPreviousFd > 0) {
                NewRes->fileInUse(iPreviousFd);
            } else {
                // Normal operations
                NewRes->file(iOPResult, ResponseMessage);
            }
        } catch (const std::string &e) {
            // Failed.
            NewRes->fail(e);
            continue;
        }
    }  // core client loop ends

    ServerUtils::buoy("Connection closed: Flie Client");

    delete NewClient;
    delete NewRes;
}

std::function<bool(const int &, const std::string &Message)> HandleSync(
    std::vector<SyncPoint> &PeersStash, const std::string &request,
    const std::string ReqType) {
    typedef std::future<std::string> PeerFuture;

    std::vector<PeerFuture> PSTash;
    std::vector<std::string> ResStash;

    // Sync With Peers
    for (auto &Peer : PeersStash) {
        PeerFuture GetFromPeer =
            std::async([&]() { return Peer.SendAndRread(request); });

        PSTash.push_back(std::move(GetFromPeer));
    }
    // Get results all in one.
    for (int i = 0; i < PSTash.size(); i++) {
        std::string res = PSTash.at(i).get();

        // Filter out errors
        if (res.substr(0, 3) == "ER-") {
            // throw res;
        }

        ResStash.push_back(res);
    }

    if (ReqType == "SYNCWRITE") {
        return [ResStash](const int &OPResult, const std::string &Message) {
            return true;
        };
    }

    if (ReqType == "SYNCREAD") {
        return [ResStash](const int &OPResult, const std::string &Message) {
            int Vote{0};
            int Total{ResStash.size()};
            //
            for (const std::string res : ResStash) {
                auto Tokenized = Lib::TokenizeDeluxe(res);
                if (Tokenized.size() < 2) {
                    continue;
                }
                int peerRead;
                try {
                    peerRead = {std::stoi(Tokenized.at(1))};
                } catch (const std::exception &e) {
                    ServerUtils::buoy(e.what());
                    continue;
                }
                // read bits and content must be the same.
                if (peerRead == OPResult && Message == Tokenized.at(2)) {
                    ++Vote;
                }
            }

            if (Vote == Total || Vote > (Total / 2)) return true;
            return false;
        };
    }

    if (ReqType == "SYNCSEEK") {
        return [ResStash](const int &OPResult, const std::string &Message) {
            int Vote{0};
            int Total{ResStash.size()};
            //
            for (const std::string res : ResStash) {
                auto Tokenized = Lib::TokenizeDeluxe(res);
                if (Tokenized.size() < 2) {
                    continue;
                }
                int peerRead;
                try {
                    peerRead = {std::stoi(Tokenized.at(1))};
                } catch (const std::exception &e) {
                    ServerUtils::buoy(e.what());
                    continue;
                }
                // read bits must be the same
                if (peerRead == OPResult) {
                    ++Vote;
                }
            }

            if (Vote == Total || Vote > (Total / 2)) return true;
            return false;
        };
    }

    throw "ER-SYNC";
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
            case 'p': {
                // peers
                std::string opts(optarg);
                std::vector<char *> tokenized = Lib::Tokenize(opts);

                for (auto ele : tokenized) {
                    sockaddr_in serv_addr;
                    serv_addr.sin_family = AF_INET;

                    std::istringstream ss(ele);
                    std::string tmp{};

                    int i{0};    // 0 = host; 1 = port
                    int err{0};  // error building the server addr.
                    while (std::getline(ss, tmp, ':')) {
                        if (i == 0) {
                            // convert the address to binary
                            if (inet_pton(AF_INET, tmp.c_str(),
                                          &serv_addr.sin_addr) <= 0) {
                                // converting error
                                ++err;
                            }
                        } else {
                            int PortNum = atoi(tmp.c_str());
                            if (PortNum <= 0) {
                                // converting error
                                ++err;
                                continue;
                            }
                            serv_addr.sin_port = htons(PortNum);
                        }
                        ++i;
                    }

                    if (err == 0) {
                        ServerUtils::PeersAddr.push_back(serv_addr);
                    }
                }

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
            case 'v': {
                // debug
                ServerUtils::bIsNoisy = true;
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
    ServerUtils::buoy("Server Quiting... ");

    // close master sockets.
    std::array<int, 2> Socks = ServerUtils::getSocketsRefList();
    for (auto i : Socks) {
        // close(i);
        shutdown(i, SHUT_RD);
        close(i);
    }
    // set the flag
    ThreadsMan::KillIdleThreads();

    // Kill all on going connections.
    ThreadsMan::CloseAllSSocks();

    FileClient::CleanUp();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // QUIT
    _Exit(EXIT_SUCCESS);
}

void HandleSIGHUP(int sig) {
    ServerUtils::rowdy("SIGHUP");
    ServerUtils::buoy("Server Restarting... ");

    // set the flag
    ThreadsMan::KillIdleThreads();

    // close master sockets.
    std::array<int, 2> Socks = ServerUtils::getSocketsRefList();
    for (auto i : Socks) {
        // close(i);
        shutdown(i, SHUT_RD);
        close(i);
    }

    // Kill all on going connections.
    ThreadsMan::CloseAllSSocks();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // clean up operations
    ThreadsMan::RestThreadsCounters();
    FileClient::CleanUp();

    ThreadsMan::StopKillIdles();
    StartServer();
    // std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // create threads
    ServerUtils::setSigHupFlag();
    ThreadsMan::NeedMoreThreads.notify_one();
    ServerUtils::rowdy("Server Restarted");
    return;
};