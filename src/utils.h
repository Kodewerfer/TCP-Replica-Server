#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/*** Error codes: ***/

/* See below for what they mean. */
const int err_host = -1;
const int err_sock = -2;
const int err_connect = -3;
const int err_proto = -4;
const int err_bind = -5;
const int err_listen = -6;

namespace utils {
/*
 * Helper function, contains the common code for passivesocket and
 * controlsocket (which are identical except for the IP address they
 * bind to).
 */
int passivesockaux(const unsigned short port, const int backlog,
                   const unsigned long int ip_addr);
/*
 * Create the master socket for the server to listen to. *
 */
int CreateSocketMaster(const unsigned short port, const int queue);
}  // namespace utils
