#include "ThreadsMan.hpp"

std::vector<int> ThreadsMan::SSocksRef;
std::mutex ThreadsMan::SSocksLock;

std::atomic<bool> ThreadsMan::bThreadKillSwitch{false};
std::atomic<bool> ThreadsMan::bThreadQuitSwitch{false};

std::atomic<int> ThreadsMan::ThreadsCount{0};
std::atomic<int> ThreadsMan::ActiveThreads{0};
std::atomic<int> ThreadsMan::QuitingThreads{0};
// std::mutex ThreadsMan::QuitingLock;
bool ThreadsMan::tryThreadQuitting() {
    if (QuitingThreads + 1 > T_incr) {
        return false;
    }
    bThreadQuitSwitch = true;
    QuitingThreads += 1;
    if (QuitingThreads == T_incr) {
        bThreadQuitSwitch = false;
        ThreadsCount -= QuitingThreads;
        QuitingThreads = 0;
    }
    return true;
}

int ThreadsMan::T_incr;

void ThreadsMan::addSScokRef(int fd) {
    std::lock_guard<std::mutex> AddGuard(SSocksLock);
    SSocksRef.push_back(fd);
}

void ThreadsMan::removeSScokRef(int fd) {
    std::lock_guard<std::mutex> RemoveGuard(SSocksLock);
    for (int i = 0; i < SSocksRef.size(); i++) {
        if (SSocksRef.at(i) == fd) {
            SSocksRef.erase(SSocksRef.begin() + i);
            break;
        }
    }
}
void ThreadsMan::CloseAllSSocks() {
    for (auto fd : SSocksRef) {
        ServerUtils::rowdy("Closing " + std::to_string(fd));
        shutdown(fd, SHUT_RD);
        close(fd);
    }
}

std::vector<std::thread> ThreadsMan::ThreadStash;
std::condition_variable ThreadsMan::NeedMoreThreads;

void ThreadsMan::ThreadCreated() { ThreadsCount += 1; }

void ThreadsMan::setActiveAndNotify() {
    ActiveThreads += 1;
    ServerUtils::rowdy("A Threads is Active.");
    ServerUtils::rowdy("Threads Count now : " +
                       std::to_string(getThreadsCount()));
    ServerUtils::rowdy("Threads Active now : " +
                       std::to_string(getActiveThreads()));
    /**
     *  If current threads all used up
     *  Notify main thread to create more
     * */

    NeedMoreThreads.notify_one();
}
void ThreadsMan::notActive() {
    ActiveThreads -= 1;
    ServerUtils::rowdy("A Threads is Deactived.");
    ServerUtils::rowdy("Threads Count now : " +
                       std::to_string(getThreadsCount()));
    ServerUtils::rowdy("Threads Active now : " +
                       std::to_string(getActiveThreads()));
    const int N = ThreadsMan::getThreadsCount();
    const int N_Active = ThreadsMan::getActiveThreads();
    const int T_incr = T_incr;

    if (N > T_incr && N_Active < (N - T_incr - 1)) {
        ServerUtils::rowdy("++ Ready to clean ++");
    }
}

void ThreadsMan::ForeRunner(ServerSockets ServSockets,
                            std::function<void(const int)> ShellCallback,
                            std::function<void(const int)> FileCallback) {
    sockaddr_in ClientAddrSTR;
    unsigned int iClientAddrLen = sizeof(ClientAddrSTR);

    int iSockets[2]{ServSockets.shell, ServSockets.file};
    //
    const int POLL_TIME_OUT{60000};

    /**
     *  Thread's main loop
     *  - handle client
     *  - clean up thread if needed
     * */
    while (true) {
        std::function<void(const int)> TheCallback_PTR{nullptr};
        AcceptedSocket AcceptedSocket;

        /**
         * Run callbacks
         * */
        try {
            AcceptedSocket = ServerUtils::PollEither(
                (int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
                (socklen_t *)&iClientAddrLen,
                POLL_TIME_OUT);  // timeout set to 1min.
            // save a reference.
            addSScokRef(AcceptedSocket.newsocket);
            // Handle the request, invoke callback.
            if (AcceptedSocket.accepted != -1 &&
                AcceptedSocket.newsocket != -1) {
                // active status
                ThreadsMan::setActiveAndNotify();
                if (AcceptedSocket.accepted == ServSockets.shell) {
                    // Shell server
                    ServerUtils::buoy("Shell client incoming");
                    TheCallback_PTR = ShellCallback;

                } else if (AcceptedSocket.accepted == ServSockets.file) {
                    // File server
                    ServerUtils::buoy("File client incoming");
                    TheCallback_PTR = FileCallback;
                }

                TheCallback_PTR(AcceptedSocket.newsocket);
                // connection closed
                removeSScokRef(AcceptedSocket.newsocket);
                close(AcceptedSocket.newsocket);
                shutdown(AcceptedSocket.newsocket, 1);
                // active stauts
                ThreadsMan::notActive();
            }
        } catch (const char *e) {
            // No data or Poll error

            // For testing
            // ServerUtils::buoy(std::string(e));
        }

        // Quit immediately if quiting.
        if (bThreadKillSwitch) {
            break;
        }

        /**
         * Thread cleanup
         * */
        // FIXME:
        // try resume quiting
        if (bThreadQuitSwitch) {
            if (tryThreadQuitting()) {
                ServerUtils::rowdy("Thread Quiting..");
                break;
            }
        }

        // try to initilize the clean up
        const int N{ThreadsMan::getThreadsCount()};
        const int N_Active{ThreadsMan::getActiveThreads()};
        const int Tincr{T_incr};

        if (N > Tincr && N_Active < (N - Tincr - 1)) {
            if (tryThreadQuitting()) {
                // std::this_thread::sleep_for(std::chrono::seconds(20));
                ServerUtils::rowdy("Initiate Quiting..");
                break;
            }
        }
    }
}