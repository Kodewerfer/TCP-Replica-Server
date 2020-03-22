#include "Utils.hpp"

bool Utils::bRunningBackground{true};
bool Utils::bIsDebugging{false};

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

Accepted Utils::AcceptAny(int *fds, int count, sockaddr *addr,
                          socklen_t *addrlen) {
    fd_set readfds;
    int maxfd, fd;
    unsigned int i;
    int status;

    FD_ZERO(&readfds);
    maxfd = -1;
    for (i = 0; i < count; i++) {
        FD_SET(fds[i], &readfds);
        if (fds[i] > maxfd) maxfd = fds[i];
    }
    status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
    if (status < 0) return {-1, -1};
    fd = -1;
    for (i = 0; i < count; i++)
        if (FD_ISSET(fds[i], &readfds)) {
            fd = fds[i];
            break;
        }
    if (fd == -1)
        return {-1, -1};
    else
        return {fd, accept(fd, addr, addrlen)};
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