#include "ThreadsMan.hpp"

std::atomic<int> ThreadsMan::ThreadsCount{0};
std::atomic<int> ThreadsMan::ThreadsActive{0};

std::vector<std::thread> ThreadsMan::ThreadStash;
std::condition_variable ThreadsMan::condCreateMore;

void ThreadsMan::incrThreadsCount() { ThreadsCount += 1; }
void ThreadsMan::decrThreadsCount() { ThreadsCount -= 1; }

void ThreadsMan::setActiveAndNotify() {
    ThreadsActive += 1;
    ServerUtils::rowdy("A Threads is Active.");
    ServerUtils::rowdy("Threads Count now : " +
                       std::to_string(getThreadsCount()));
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
                       std::to_string(getThreadsCount()));
    ServerUtils::rowdy("Threads Active now : " +
                       std::to_string(getActiveThreads()));
}

void ThreadsMan::ForeRunner(ServerSockets ServSockets,
                            std::function<void(const int)> ShellCallback,
                            std::function<void(const int)> FileCallback) {
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
        Accepted AcceptedSocket;
        const int POLL_TIME_OUT{60000};

        /**
         * Run callbacks
         * */
        try {
            AcceptedSocket = ServerUtils::PollEither(
                (int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
                (socklen_t *)&iClientAddrLen,
                POLL_TIME_OUT);  // timeout set to 1min.

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
                // active stauts
                ThreadsMan::notActive();
            }
        } catch (const char *e) {
            // No data or Poll error

            // For testing
            // ServerUtils::buoy(std::string(e));
        }

        // No data or time out proceed to clean up

        /**
         * Thread cleanup
         * */
    }
}