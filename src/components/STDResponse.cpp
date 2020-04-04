#include "STDResponse.hpp"

STDResponse::STDResponse(int fd)
    : ClientFd(fd),
      ERROR_CODES({{"ERPIP", "Error Creating The Pipe"},
                   {"ER-F-OP", "Error Opening The File"},
                   {"ER-F-ACEDI", "File Access Control : Action Denied"},
                   {"ER-F-FD", "File Already Opened"},
                   {"ER-SYNC", "Syncing Failed."},
                   {"ER-SYNC-SOC", "Connection Socket Error"},
                   {"ER-SYNC-CONN", "Connection To Peers Failed"}}) {}

void STDResponse::sendPayload(std::string &Content) {
    Content += "\n";
    send(ClientFd, Content.c_str(), Content.size(), 0);
}

void STDResponse::file(int code, std::string message) {
    bool bIsSuccess{code >= 0};
    std::string Payload = "";

    // stat
    if (bIsSuccess)
        Payload += "OK ";
    else
        Payload += "ERR ";

    // code
    if (bIsSuccess) Payload += std::to_string(code) + " ";
    // if (!bIsSuccess||) Payload += "ENOENT ";
    if (!bIsSuccess) Payload += "ENOENT ";

    // message
    if (message != " ") {
        Payload += message;
    } else {
        if (bIsSuccess) Payload += "Operation Success";

        if (!bIsSuccess && code == NO_SUCH_FILE)
            Payload += "No Such File Or Directory";
        if (!bIsSuccess && code == NO_PRE_OEPN)
            Payload += "No Previously Opened File";
        if (!bIsSuccess && code == PARAM_PARS) Payload += "Param Parsing Error";
        if (!bIsSuccess && code == WRT_FAILED) Payload += "Writing Failed";
        if (!bIsSuccess && code == NO_VALID_COM)
            Payload += "No Valid Command Received";
    }

    sendPayload(Payload);

    return;
}

void STDResponse::fileFdInUse(int fd) {
    std::string Payload = "ERR ";
    Payload += std::to_string(fd) + " ";
    Payload += "This File Has Already Been Opened";

    sendPayload(Payload);
}

// ERR or OK
void STDResponse::shell(int code, std::string message) {
    std::string Payload = "";
    // stat
    if (code == 0) Payload += "OK ";

    if (code != 0) Payload += "ERR ";

    // code
    if (code == -3)
        Payload += "EIO ";
    else
        Payload += std::to_string(code) + " ";

    // message
    if (message != " ")
        Payload += message;
    else if (code == -3)
        Payload += "No Command Issued";
    else if (code == 254)
        Payload += "Program Cannot Be Found";
    else if (code != 0)
        Payload += "Non-zero Exit";
    else
        Payload += "Success";

    sendPayload(Payload);

    return;
}

// FAIL only
void STDResponse::fail(std::string ServerCode) {
    std::string Payload = "FAIL " + ServerCode + " ";

    Payload += ERROR_CODES[ServerCode];

    sendPayload(Payload);

    return;
}