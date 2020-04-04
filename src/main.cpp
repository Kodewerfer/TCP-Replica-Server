#include "main.hpp"

#define SHELL_DEFAULT 9000;
#define FILE_DEFAULT 9001;
#define THREAD_INCR_DEFAULT 2;
#define THREAD_MAX_DEFAULT 4;

int main(int arg, char *argv_main[], char *envp[]) {
    OptParsed FromOpts = ParsOpt(arg, argv_main, "f:s:T:t:p:dD");
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

    if (ServerUtils::PeersAddr.size() > 0) {
        ServerUtils::buoy(std::to_string(ServerUtils::PeersAddr.size()) +
                          " Peers found, running in Replica mode.");
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
    std::string WelcomeMessage = "Shell Server Connected.";

    // NEW - Shell client limit
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
    }  // core client loop ends

    ServerUtils::buoy("Connection closed: Shell Client.");

    delete NewClient;
    delete NewRes;
}

void DoFileCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];

    FileClient *NewClient = new FileClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    // if the server is syncing with other.
    const bool bSendSyncRequests{ServerUtils::PeersAddr.size() > 0};

    int n{0};
    while (n = (Lib::readline(iServFD, req, ALEN - 1)) >= 0) {
        const std::string sRequest(req);

        std::vector<char *> RequestTokenized = Lib::Tokenize(sRequest);
        char *TheCommand{RequestTokenized.at(0)};

        // Important parameters
        int OPResult{NO_VALID_COM};
        std::string ResponseMessage{" "};
        std::function<bool(const int &, const std::string &)> SyncCallback;
        //
        try {
            /**
             * Local Operations
             * */
            // FOPEN
            int previousFd{-1};
            if (strcmp(TheCommand, "FOPEN") == 0) {
                OPResult = NewClient->FOPEN(RequestTokenized, previousFd);
            }
            // FSEEK
            if (strcmp(TheCommand, "FSEEK") == 0) {
                OPResult = NewClient->FSEEK(RequestTokenized);
            }
            // FREAD
            if (strcmp(TheCommand, "FREAD") == 0) {
                // original
                // read request return file content in an out param.
                OPResult = NewClient->FREAD(RequestTokenized, ResponseMessage);
                // sync wouldn't run if local request reported error
                if (bSendSyncRequests && OPResult >= 0) {
                    // Build the sync request
                    std::string SyncRequest =
                        NewClient->SyncRequestBuilder(RequestTokenized);
                    // send the sync requests.
                    SyncCallback = HandleSync(SyncRequest, "SYNCREAD");
                }
            }
            // FWRITE
            if (strcmp(TheCommand, "FWRITE") == 0) {
                // local request
                OPResult = NewClient->FWRITE(RequestTokenized);
                // sync wouldn't run if local request reported error
                if (bSendSyncRequests && OPResult >= 0) {
                    // Build the sync request
                    std::string SyncRequest =
                        NewClient->SyncRequestBuilder(RequestTokenized);
                    // send the sync requests.
                    SyncCallback = HandleSync(SyncRequest, "SYNCWRITE");
                }
            }
            // FCLOSE
            if (strcmp(TheCommand, "FCLOSE") == 0) {
                OPResult = NewClient->FCLOSE(RequestTokenized);
            }

            /**
             * Receiving end of Sync Operations.
             * */
            if (strcmp(TheCommand, "SYNCWRITE") == 0) {
                OPResult = NewClient->SYNCWRITE(RequestTokenized);
            }
            if (strcmp(TheCommand, "SYNCREAD") == 0) {
                OPResult =
                    NewClient->SYNCREAD(RequestTokenized, ResponseMessage);
            }

            /**
             * Send the response to user
             * */
            if (bSendSyncRequests && SyncCallback != nullptr) {
                // origin of the sync requests.
                // pass on the result from local operation.
                if (SyncCallback(OPResult, ResponseMessage)) {
                    NewRes->file(OPResult, ResponseMessage);
                } else {
                    throw std::string("ER-SYNC-READ");
                }
            } else if (previousFd > 0) {
                NewRes->fileFdInUse(previousFd);
            } else {
                // Normal operations
                NewRes->file(OPResult, ResponseMessage);
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
    const std::string &request, const std::string ReqType) {
    typedef std::future<std::string> PeerFuture;

    std::vector<PeerFuture> PSTash;
    std::vector<std::string> ResStash;

    // Sync With Peers
    for (auto &serv_addr : ServerUtils::PeersAddr) {
        PeerFuture GetFromPeer = std::async([&]() {
            int sock{-1};
            char buffer[256]{0};

            // create socket to peer
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                return std::string("ER-SYNC-SOC");
            }
            // make connection
            if (connect(sock, (struct sockaddr *)&serv_addr,
                        sizeof(serv_addr)) < 0) {
                return std::string("ER-SYNC-CONN");
            }

            // send to the peer
            send(sock, request.c_str(), request.size(), 0);
            // Close connection
            shutdown(sock, SHUT_WR);

            // Poll response
            const int POLL_TIME_OUT{ServerUtils::bIsDebugging ? 3500 : 500};
            Lib::recv_nonblock(sock, buffer, 256, POLL_TIME_OUT);

            close(sock);

            return std::string(buffer);
        });

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
            const int Total{ResStash.size()};
            //
            for (const std::string res : ResStash) {
                auto Tokenized = Lib::TokenizeDeluxe(res);
                int peerRead{std::stoi(Tokenized.at(1))};
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
            const int Total{ResStash.size()};
            //
            for (const std::string res : ResStash) {
                auto Tokenized = Lib::TokenizeDeluxe(res);
                int peerRead{std::stoi(Tokenized.at(1))};
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
    std::array<int, 2> Socks = ServerUtils::getSocketsRef();
    for (auto i : Socks) {
        // close(i);
        shutdown(i, SHUT_RD);
        close(i);
    }
    // set the flag
    ThreadsMan::StartServerQuiting();

    // Kill all on going connections.
    ThreadsMan::CloseAllSSocks();

    std::this_thread::sleep_for(std::chrono::seconds(1));
    // QUIT
    _Exit(EXIT_SUCCESS);
}

void HandleSIGHUP(int sig) {
    ServerUtils::rowdy("SIGHUP");
    ServerUtils::buoy("Server Restarting... ");

    // set the flag
    ThreadsMan::StartServerQuiting();

    // Kill all on going connections.
    ThreadsMan::CloseAllSSocks();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    ThreadsMan::RestCounters();
    ThreadsMan::StopServerQuiting();
    // std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // create threads
    ServerUtils::setSigHupFlag();
    ThreadsMan::NeedMoreThreads.notify_one();
    ServerUtils::rowdy("Server Restarted");
    return;
};