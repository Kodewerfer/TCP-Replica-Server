/**
 * Assignment for csc464/564

 *by

 * Jialin Li 002250729
 * Sun Jian 002244864
 **/

#include "main.hpp"

int main(int arg, char *argv_main[], char *envp[]) {
    const int iShPort{9000};
    const int iFiPort{9001};

    signal(SIGPIPE, SigPipeHandle);

    ServerSockets ServSockets;
    try {
        ServSockets = InitServer();
        PrintMessage(iShPort, iFiPort);
    } catch (char const *msg) {
        Utils::buoy(msg);
    }

    StartServer(ServSockets, DoShellCallback, DoFileCallback);

    return EXIT_SUCCESS;
}

/**
 *
 * Main logics
 *
 **/
ServerSockets InitServer() {
    // init shell server.
    int iShServSocket{-1};
    int iFiServSocket{-1};

    iShServSocket = Utils::CreateSocketMaster(9000);
    iFiServSocket = Utils::CreateSocketMaster(9001);

    return {iShServSocket, iFiServSocket};
}

void PrintMessage(const int iSh, const int iFi) {
    Utils::buoy("SHFD Shell and file server.");
    Utils::buoy("Shell Server is listening on port " + std::to_string(iSh));
    Utils::buoy("SHFD Shell and file server. " + std::to_string(iFi));
}

void StartServer(ServerSockets ServSockets,
                 std::function<void(const int)> ShellCallback,
                 std::function<void(const int)> FileCallback) {
    if (ServSockets.shell < 0 || ServSockets.file < 0) {
        Utils::buoy("Server unable to start.");
        return;
    }

    sockaddr_in ClientAddrSTR;
    unsigned int iClientAddrLen = sizeof(ClientAddrSTR);

    // loop accepting
    int iSockets[2]{ServSockets.shell, ServSockets.file};
    Accepted accepted_STR =
        Utils::AcceptAny((int *)iSockets, 2, (sockaddr *)&ClientAddrSTR,
                         (socklen_t *)&iClientAddrLen);

    if (accepted_STR.accepted == ServSockets.shell) {
        Utils::buoy("Shell client incoming");
        ShellCallback(accepted_STR.newsocket);

    } else if (accepted_STR.accepted == ServSockets.file) {
        Utils::buoy("File client incoming");
        FileCallback(accepted_STR.newsocket);

    } else {
        Utils::buoy("Failed to accept any client");
    }

    close(accepted_STR.newsocket);
    shutdown(accepted_STR.newsocket, 1);
}

/**
 *
 * Callback functions
 *
 **/
void DoShellCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];

    ShellClient *NewClient = new ShellClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    while ((lib::readline(iServFD, req, ALEN - 1)) != FLAG_NO_DATA) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = lib::Tokenize(sRequest);

        if (strcmp(RequestTokenized.at(0), "CPRINT") == 0) {
            std::string &LastOutput = NewClient->GetLastOutput();
            if (LastOutput == "MARK-NO-COMMAND") {
                NewRes->shell(-3);
                continue;
            }
            send(iServFD, LastOutput.c_str(), LastOutput.size(), 0);
            NewRes->shell(0);
            continue;
        }
        int ShellRes;
        try {
            ShellRes = NewClient->RunShellCommand(RequestTokenized);
            NewRes->shell(ShellRes);
        } catch (const std::string &e) {
            NewRes->fail(e);
            Utils::buoy(e);
        }
    }
    Utils::buoy("Shell connection closed by client.");
    shutdown(iServFD, 1);
}

void DoFileCallback(const int iServFD) {
    const int ALEN = 256;
    char req[ALEN];

    FileClient *NewClient = new FileClient();
    STDResponse *NewRes = new STDResponse(iServFD);

    while ((lib::readline(iServFD, req, ALEN - 1)) != FLAG_NO_DATA) {
        const std::string sRequest(req);
        std::vector<char *> RequestTokenized = lib::Tokenize(sRequest);
        char *TheCommand{RequestTokenized.at(0)};

        int res{-5};
        std::string message{" "};
        try {
            // FOPEN
            if (strcmp(TheCommand, "FOPEN") == 0) {
                res = NewClient->FOPEN(RequestTokenized);
            }
            // FSEEK
            if (strcmp(TheCommand, "FSEEK") == 0) {
                res = NewClient->FSEEK(RequestTokenized);
            }
            // FREAD
            if (strcmp(TheCommand, "FREAD") == 0) {
                res = NewClient->FREAD(RequestTokenized, message);
            }
            // FWRITE
            if (strcmp(TheCommand, "FWRITE") == 0) {
                res = NewClient->FWRITE(RequestTokenized);
            }
            // FCLOSE
            if (strcmp(TheCommand, "FCLOSE") == 0) {
                res = NewClient->FCLOSE(RequestTokenized);
            }
        } catch (const std::string &e) {
            NewRes->fail(e);
            continue;
        }

        NewRes->file(res, message);
    }
    Utils::buoy("File Connection closed by client.");
    shutdown(iServFD, 1);
}

/**
 *
 * Miscs
 *
 **/
void SigPipeHandle(int signum) { return; }