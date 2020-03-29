#include "Utils.hpp"

bool Utils::bRunningBackground{true};
bool Utils::bIsDebugging{false};
std::mutex Utils::ShellServerLock;

int Utils::CreateSocket(const unsigned short port,
                        const unsigned long int ip_addr, const int backlog) {
    int iSocketFD;
    int opt{1};
    //  create socket
    if ((iSocketFD = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw "Socket Creation Error.";
        return -1;
    }
    if (setsockopt(iSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        close(iSocketFD);
        throw "Setsockopt Error.";
        return -1;
    }

    // binding
    sockaddr_in strAddress;
    strAddress.sin_family = AF_INET;
    strAddress.sin_addr.s_addr = htonl(ip_addr);

    strAddress.sin_port = (unsigned short)htons(port);

    if (bind(iSocketFD, (sockaddr *)&strAddress, sizeof(strAddress)) < 0) {
        close(iSocketFD);
        throw "Binding socket error.";
    }
    if (listen(iSocketFD, backlog) < 0) {
        close(iSocketFD);
        throw "Listening error.";
    }
    return iSocketFD;
}

int Utils::CreateSocketMaster(const unsigned short port, const int queue) {
    return CreateSocket(port, INADDR_ANY, queue);
}

int Utils::CreateSocketMasterLocalOnly(const unsigned short port,
                                       const int queue) {
    return CreateSocket(port, INADDR_LOOPBACK, queue);
}

Accepted Utils::PollEither(int *fds, int count, sockaddr *addr,
                           socklen_t *addrlen, int TimeOut) {
    pollfd PollFromSocks[count]{0};

    for (int i = 0; i < count; i++) {
        PollFromSocks[i].fd = fds[i];
        PollFromSocks[i].events = POLLIN;
    }

    int polled = poll(PollFromSocks, count, TimeOut);

    if (polled == 0) throw "NODATA";
    if (polled == -1) throw "ERRPOL";

    // return recv(sd, buf, max, 0);

    int ActiveFD{-2};

    if (PollFromSocks[0].revents & POLLIN) {
        ActiveFD = PollFromSocks[0].fd;
    }

    if (PollFromSocks[1].revents & POLLIN) {
        ActiveFD = PollFromSocks[1].fd;
    }

    return {ActiveFD, accept(ActiveFD, addr, addrlen)};
}

int recv_nonblock(int sd, char *buf, size_t max, int timeout) {
    struct pollfd pollrec;
    pollrec.fd = sd;
    pollrec.events = POLLIN;

    int polled = poll(&pollrec, 1, timeout);

    if (polled == 0) return FLAG_NO_DATA;
    if (polled == -1) return -1;

    return recv(sd, buf, max, 0);
}

void Utils::buoy(std::string message) {
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

void Utils::rowdy(std::string message) {
    if (!bIsDebugging) {
        return;
    }

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

std::string Utils::GetTID() {
    auto myid = std::this_thread::get_id();
    std::stringstream ss;
    ss << myid;
    std::string mystring = ss.str();

    return mystring;
}