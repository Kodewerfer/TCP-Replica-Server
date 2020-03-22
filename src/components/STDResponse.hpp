#pragma once

// #include <socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "FileErrCode.hpp"

#include <map>
#include <string>



class STDResponse {
   private:
    // SERVER ERROR CODE
    std::map<std::string, std::string> ERROR_CODES;
    int ClientFd;
    void sendPayload(std::string&);

   public:
    STDResponse(int fd);
    void file(int code, std::string messages = " ");
    // void file(std::messages);
    void shell(int, std::string message = " ");
    void fail(std::string ServerCode);
};