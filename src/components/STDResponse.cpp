#include "STDResponse.hpp"

STDResponse::STDResponse(int fd)
    : ClientFd(fd),
      ERROR_CODES({{
          "ERPIP",
          "Error creating the pipe",
      }}) {}

void STDResponse::sendPayload(std::string &Content) {
    send(ClientFd, Content.c_str(), Content.size(), 0);
    send(ClientFd, "\n", 1, 0);
}

// ERR or OK
void STDResponse::shell(int code, std::string message) {
    std::string Payload = "";
    // stat
    if (code == 0) {
        Payload += "OK ";
    }
    if (code != 0 && code != 254) {
        Payload += "ERR ";
    }
    if (code == 254) {
        Payload += "FAIL ";
    }

    // code
    if (code == -3) {
        Payload += "EIO ";
    } else {
        Payload += std::to_string(code) + " ";
    }

    // message
    if (message != " ") {
        Payload += message;
    } else if (code == -3) {
        Payload += "No Command Issued";
    } else if (code == 254) {
        Payload += "Program Cannot Be Found";
    } else if (code != 0) {
        Payload += "Non-zero Exit";
    } else {
        Payload += "Success";
    }

    sendPayload(Payload);

    return;
}

// FAIL only
void STDResponse::fail(std::string ServerCode) {
    std::string Payload = "FAIL " + ServerCode + " ";

    for (auto const &ele : ERROR_CODES) {
        if (ele.first == ServerCode) {
            Payload += ele.second;
        }
    }

    sendPayload(Payload);

    return;
}