#include "SyncPoint.hpp"

SyncPoint::SyncPoint() : bIsDead(false) {}
SyncPoint::SyncPoint(const SyncPoint &source)
    : bIsDead(source.bIsDead), PeerSock(source.PeerSock) {}
SyncPoint::SyncPoint(SyncPoint &&source)
    : bIsDead(source.bIsDead), PeerSock(source.PeerSock) {
    source.PeerSock = 0;
    source.bIsDead = false;
}

void SyncPoint::init(sockaddr_in PeerAddress) {
    ServerUtils::rowdy("Connecting To Peer...");
    // create socket to peer
    if ((PeerSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        // throw std::string("ER-SYNC-SOC");
        throw std::string("ER-SYNC-SOC");
    }
    // make connection
    if (connect(PeerSock, (struct sockaddr *)&PeerAddress,
                sizeof(PeerAddress)) < 0) {
        throw std::string("ER-SYNC-CONN");
    }
}

std::string SyncPoint::SendAndRread(std::string request) {
    char buffer[256]{0};

    ServerUtils::rowdy("Sending Request to Peer...");
    // send to the peer
    send(PeerSock, request.c_str(), request.size(), 0);
    // Poll response
    const int POLL_TIME_OUT{ServerUtils::bIsDebugging ? 3500 : 500};
    Lib::recv_nonblock(PeerSock, buffer, 256, POLL_TIME_OUT);

    return std::string(buffer);
}

void SyncPoint::isDead() { bIsDead = true; }

SyncPoint::~SyncPoint() {
    if (!bIsDead) {
        ServerUtils::rowdy("Cleaning Peer Connection...");
        shutdown(PeerSock, 1);
        close(PeerSock);
    } else {
        ServerUtils::rowdy("Peer Connection already died.");
    }
}