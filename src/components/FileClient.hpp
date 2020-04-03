#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../ServerUtils.hpp"

// Numbered error codes
const int NO_SUCH_FILE = -1;
const int NO_PRE_OEPN = -2;
const int PARAM_PARS = -3;
const int WRT_FAILED = -4;
const int NO_VALID_COM = -5;

struct LastRequest {
    const int fd;
    const int params;
};

struct AccessCtl {
    std::atomic<int> reading{0};
    std::atomic<int> writing{0};
    std::mutex writeLock;
};

class FileClient {
    // Statics
   private:
    // file access control
    static std::map<int, std::string> FdToName;
    static std::map<std::string, int> NameToFd;
    static std::map<std::string, AccessCtl> FileAccess;

   private:
    // Stored FDs
    std::vector<int> OpenedFiles;
    // Turn fd and param(if apply) to int.
    LastRequest PreprocessReq(std::vector<char *> Request);
    /**
     *   Check if the fs is valid.
     *   If there is any file in this session been opened.
     *   If This fd has been previously opened in this session.
     * */
    int FDChcker(int fd);

    // Statics
   public:
    static bool bIsDebugging;

   public:
    int FOPEN(std::vector<char *> Request, int &outPreFd);
    int FSEEK(std::vector<char *> Request);
    int FREAD(std::vector<char *> Request, std::string &outMessage);
    int FWRITE(std::vector<char *> Request);
    int FCLOSE(std::vector<char *> Request);
    ~FileClient();
};
