#include "ThreadsMan.hpp"

std::atomic<int> ThreadsMan::ThreadsCount{0};
std::atomic<int> ThreadsMan::ThreadsActive{0};

std::vector<std::thread> ThreadsMan::ThreadStash;
std::condition_variable ThreadsMan::condCreateMore;

void ThreadsMan::incrThreadsCout() { ThreadsCount += 1; }
void ThreadsMan::decrThreadsCout() { ThreadsCount -= 1; }

void ThreadsMan::isActive() {
    ThreadsActive += 1;
    ServerUtils::rowdy("A Threads is Active.");
    ServerUtils::rowdy("Threads Count now : " +
                       std::to_string(getThreadsCout()));
    ServerUtils::rowdy("Threads Active now : " +
                       std::to_string(getActiveThreads()));
    /**
     *  If current threads all used up
     *  Notify main thread to create more
     * */

    condCreateMore.notify_one();
}
void ThreadsMan::notActive() {
    ThreadsActive -= 1;
    ServerUtils::rowdy("A Threads is Deactived.");
    ServerUtils::rowdy("Threads Count now : " +
                       std::to_string(getThreadsCout()));
    ServerUtils::rowdy("Threads Active now : " +
                       std::to_string(getActiveThreads()));
}

void ThreadsMan::ForeRunner(ServerSockets ServSockets,
                               std::function<void(const int)> ShellCallback,
                               std::function<void(const int)> FileCallback) {
    if (ServSockets.shell < 0 || ServSockets.file < 0) {
        ServerUtils::buoy("Server unable to start.");
        return;
    }
    sockaddr_in ClientAddrSTR;
    unsigned int iClientAddrLen = sizeof(ClientAddrSTR);

    /**
     *  Thread's main loop
     *  - handle client
     *  - clean up thread if needed
     * */
    while (true) {
        /**
         *  Client handle
         * */

        int iSockets[2]{ServSockets.shell, ServSockets.file};
        std::function<void(const int)> TheCallback_PTR{nullptr};
        Accepted accepted_STR;

        try {
            accepted_STR = ServerUtils::PollEither(
                (int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
                (socklen_t *)&iClientAddrLen, 60000);  // timeout set to 1min.
        } catch (const char *e) {
            // No data or Poll error

            // For testing
            // ServerUtils::buoy(std::string(e));
        }
        // Handle the request, invoke callback.
        if (accepted_STR.accepted != -1 && accepted_STR.newsocket != -1) {
            // active status
            ThreadsMan::isActive();
            if (accepted_STR.accepted == ServSockets.shell) {
                // Shell server
                ServerUtils::buoy("Shell client incoming");
                TheCallback_PTR = ShellCallback;

            } else if (accepted_STR.accepted == ServSockets.file) {
                // File server
                ServerUtils::buoy("File client incoming");
                TheCallback_PTR = FileCallback;
            }

            TheCallback_PTR(accepted_STR.newsocket);
            // active stauts
            ThreadsMan::notActive();
        }

        // No data or time out proceed to clean up

        /**
         * Thread cleanup
         * */
    }
}