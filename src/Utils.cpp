#include "Utils.hpp"

bool Utils::bRunningBackground{false};

int Utils::CreateSocket(const unsigned short port,
                        const unsigned long int ip_addr, const int backlog) {
    int iSocketFD;
    int opt{1};
    //  create socket
    if ((iSocketFD = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        throw "Socket Creation Error.";
    }
    if (setsockopt(iSocketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        close(iSocketFD);
        throw "Setsockopt Error.";
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

void Utils::buoy(std::string message) {
    if (bRunningBackground) {
        // log
    } else {
        // display to console
        std::cout << message << "\n";
    }
}