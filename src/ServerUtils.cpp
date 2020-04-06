#include "ServerUtils.hpp"

ServerSockets ServerUtils::SocketReference;
ServerPorts ServerUtils::PortsReference;
std::atomic<bool> ServerUtils::bSIGHUPReceived{false};

bool ServerUtils::bRunningBackground{true};
bool ServerUtils::bIsDebugging{false};
bool ServerUtils::bIsNoisy{false};
std::mutex ServerUtils::ShellServerLock;

std::vector<sockaddr_in> ServerUtils::PeersAddrs;

bool ServerUtils::trySighupFlag() {
    if (bSIGHUPReceived) {
        bSIGHUPReceived = false;
        return true;
    }

    return false;
}

void ServerUtils::setSocketsRef(ServerSockets &ref) { SocketReference = ref; };
std::array<int, 2> ServerUtils::getSocketsRefList() {
    return std::array<int, 2>{SocketReference.file, SocketReference.shell};
}

int ServerUtils::CreateSocket(const unsigned short port,
                              const unsigned long int ip_addr,
                              const int backlog) {
    int iSocketFD;
    int opt{1};
    //  create socket
    if ((iSocketFD = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw ServerException("Socket Creation Error.");
        return -1;
    }
    if (setsockopt(iSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        close(iSocketFD);
        throw ServerException("Setsockopt Error.");
        return -1;
    }

    // binding
    sockaddr_in strAddress;
    strAddress.sin_family = AF_INET;
    strAddress.sin_addr.s_addr = htonl(ip_addr);

    strAddress.sin_port = (unsigned short)htons(port);

    if (bind(iSocketFD, (sockaddr *)&strAddress, sizeof(strAddress)) < 0) {
        close(iSocketFD);
        throw ServerException("Binding socket error.");
        return -1;
    }
    if (listen(iSocketFD, backlog) < 0) {
        close(iSocketFD);
        throw ServerException("Listening error.");
        return -1;
    }
    return iSocketFD;
}

int ServerUtils::CreateSocketMaster(const unsigned short port,
                                    const int queue) {
    return CreateSocket(port, INADDR_ANY, queue);
}

int ServerUtils::CreateSocketMasterLocalOnly(const unsigned short port,
                                             const int queue) {
    return CreateSocket(port, INADDR_LOOPBACK, queue);
}

AcceptedSocket ServerUtils::PollEither(int *fds, int count, sockaddr *addr,
                                       socklen_t *addrlen, int TimeOut) {
    pollfd PollFromSocks[count]{0};
    AcceptedSocket accpeted;

    for (int i = 0; i < count; i++) {
        PollFromSocks[i].fd = fds[i];
        PollFromSocks[i].events = POLLIN;
    }

    int polled = poll(PollFromSocks, count, TimeOut);

    if (polled == 0) throw AcceptingException("NO DATA");
    if (polled == -1) throw AcceptingException("Poll Error");

    // return recv(sd, buf, max, 0);

    int ActiveFD{-2};

    if (PollFromSocks[0].revents & POLLIN) {
        ActiveFD = PollFromSocks[0].fd;
    } else if (PollFromSocks[1].revents & POLLIN) {
        ActiveFD = PollFromSocks[1].fd;
    }

    if (ActiveFD == -2) throw "IMPOSB";

    accpeted.accepted = ActiveFD;
    accpeted.newsocket = accept(ActiveFD, addr, addrlen);
    return accpeted;
}

std::string ServerUtils::GetTID() {
    auto myid = std::this_thread::get_id();
    std::stringstream ss;
    ss << myid;
    std::string mystring = ss.str();

    return mystring;
}

// TODO: better logging
void ServerUtils::buoy(std::string message) {
    std::time_t result = std::time(nullptr);

    if (bRunningBackground) {
        // log
        std::ofstream log;
        log.open("shfd.log", std::ios::app);
        log << std::asctime(std::localtime(&result));
        log << message << "\n"
            << "\n";
        log.close();
    } else {
        // display to console
        std::cout.flush();
        std::cout << std::asctime(std::localtime(&result)) << "  : " << message
                  << "\n";
    }
}

void ServerUtils::rowdy(std::string message) {
    if (!bIsNoisy) {
        return;
    }

    std::time_t result = std::time(nullptr);

    if (bRunningBackground) {
        // log
        std::ofstream log;
        log.open("shfd.log", std::ios::app);
        log << "DEBUG -- " << message << "\n";
        log.close();
    } else {
        // display to console
        std::cout.flush();
        std::cout << "DEBUG -- " << message << "\n";
    }
}