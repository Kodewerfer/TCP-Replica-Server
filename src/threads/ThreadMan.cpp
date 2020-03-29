#include "ThreadsMan.hpp"

int ThreadsMan::ThreadsCount{0};

void ThreadsMan::ThreadManager(ServerSockets ServSockets,
                               std::function<void(const int)> ShellCallback,
                               std::function<void(const int)> FileCallback) {
    if (ServSockets.shell < 0 || ServSockets.file < 0) {
        ServerUtils::buoy("Server unable to start.");
        return;
    }

    sockaddr_in ClientAddrSTR;
    unsigned int iClientAddrLen = sizeof(ClientAddrSTR);

    // loop accepting
    while (true) {
        int iSockets[2]{ServSockets.shell, ServSockets.file};
        // the correct callback to call;
        std::function<void(const int)> TheCallback_PTR{nullptr};
        Accepted accepted_STR = ServerUtils::PollEither(
            (int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
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

        TheCallback_PTR(accepted_STR.newsocket);
    }
}