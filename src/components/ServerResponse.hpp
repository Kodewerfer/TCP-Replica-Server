#pragma once

// #include <socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <string>

#include "../ServerUtils.hpp"
#include "FileClient.hpp"

class ServerResponse {
   private:
    // SERVER ERROR CODE
    static std::map<std::string, std::string> ERROR_CODES;

   private:
    const int ClientFd;
    // send function
    void sendPayload(std::string &);

   public:
    ServerResponse(const int fd);
    // For file err or ok
    void file(const int code, std::string messages = " ");
    // only if file was opened.
    void fileInUse(const int fd);
    // For shell ok or err
    void shell(const int, std::string message = " ");
    // Failed only, used in catch
    void fail(const std::string &ServerCode);
    // Sync fail special, called alone.
     // ** unused for now **
    void syncFail();
    // display failed reason   
    void syncFail(const std::string &);
};
