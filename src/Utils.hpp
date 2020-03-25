#pragma once

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

const int FLAG_NO_DATA{-2};

struct Accepted {
    int accepted;
    int newsocket;
};

class Utils {
   private:
   public:
    static bool bRunningBackground;
    static bool bIsDebugging;
    static std::mutex ShellServerLock;
    /**
     *  set the flag for background running
     * */
    bool setRunningBackground(bool flag) { return bRunningBackground = flag; };

    /**
     * Helper function, contains the common code for passivesocket and
     * controlsocket (which are identical except for the IP address they
     * bind to).
     */
    static int CreateSocket(const unsigned short port,
                            const unsigned long int ip_addr,
                            const int backlog = 32);
    /**
     * Create the master socket for the server to listen to.
     */
    static int CreateSocketMaster(const unsigned short port,
                                  const int queue = 32);
    /**
     *  Accept avalible client.
     * */
    static Accepted AcceptAny(int *fds, int count, sockaddr *addr,
                              socklen_t *addrlen);

    /**
     *  display or logging.
     * */
    static void buoy(std::string message);
    static void rowdy(std::string message);

    static std::string GetTID();
};
