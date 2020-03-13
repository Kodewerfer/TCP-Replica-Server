/**
 * Assignment for csc464/564

 *by

 * Jialin Li 002250729
 * Sun Jian 002244864
 **/

#include "main.h"
#include "lib.h"
#include "utils.h"

int main(int arg, char *argv_main[], char *envp[]) {
    int iShPort{9000};
    int iFiPort{9001};

    if (!initServer()) {
        return -1;
    }

    printMessage(iShPort, iFiPort);

    doServer();

    return 0;
}

void printMessage(int iSh, int iFi) {
    std::cout << "###############################################"
              << "\n"
              << "\n";
    std::cout << "- SHFD Shell and file server."
              << "\n"
              << "\n";
    std::cout << "###############################################"
              << "\n";

    std::cout << "Shell Server is listening on port" + iSh << "\n";
    std::cout << "File Server is listening on port" + iFi << "\n";
}

bool initServer() { return true; }

void doServer() {}