#pragma once

#include "utils.h"

namespace utils {
int passivesockaux(const unsigned short port, const int backlog,
                   const unsigned long int ip_addr) {
    struct sockaddr_in sin;        // address to connect to
    int sd;                        // socket description to be returned
    const int type = SOCK_STREAM;  // TCP connection

    memset(&sin, 0, sizeof(sin));  // needed for correct padding... (?)
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(ip_addr);

    sin.sin_port = (unsigned short)htons(port);

    // allocate socket:
    sd = socket(PF_INET, type, 0);
    if (sd < 0) return err_sock;

    // bind socket:
    if (bind(sd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        close(sd);
        return err_bind;
    }
    // socket is listening:
    if (listen(sd, backlog) < 0) {
        close(sd);
        return err_listen;
    }

    // done!
    return sd;
}

int CreateSocketMaster(const unsigned short port, const int queue) {
    return passivesockaux(port, queue, INADDR_ANY);
}

}  // namespace utils
