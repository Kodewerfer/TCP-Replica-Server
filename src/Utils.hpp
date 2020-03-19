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

#include <iostream>
#include <string>

/*** Error codes: ***/

/* See below for what they mean. */
const int err_host = -1;
const int err_sock = -2;
const int err_connect = -3;
const int err_proto = -4;
const int err_bind = -5;
const int err_listen = -6;

const int FLAG_NO_DATA{-2};

class Utils {
   private:
    static bool bRunningBackground;

   public:
    //    set the flag for background running
    bool setRunningBackground(bool flag) { return bRunningBackground = flag; };
    /*
     * Helper function, contains the common code for passivesocket and
     * controlsocket (which are identical except for the IP address they
     * bind to).
     */
    static int CreateSocket(const unsigned short port,
                            const unsigned long int ip_addr,
                            const int backlog = 32);
    /*
     * Create the master socket for the server to listen to.
     */
    static int CreateSocketMaster(const unsigned short port,
                                  const int queue = 32);

    // display or logging.
    static void buoy(std::string message);
};
