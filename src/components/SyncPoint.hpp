#pragma once

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "../Lib.hpp"
#include "../ServerUtils.hpp"

class SyncPoint {
   private:
    int PeerSock;
    bool bIsDead;

   public:
    SyncPoint();
    // copy
    SyncPoint(const SyncPoint &source);
    // move
    SyncPoint(SyncPoint &&);
    // make the connection to peer
    void init(sockaddr_in PeerAddress);
    // send sync request and get the result.
    std::string SendAndRread(std::string request);
    // something went wrong, no longer make sending and receiving actions
    void isDead();

    ~SyncPoint();
};
