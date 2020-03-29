#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../ServerUtils.hpp"
#include "FileErrCode.hpp"

struct LastRequest {
    int fd;
    int params;
};

struct AccessCtl {
    int reading{0};
    int writing{0};
    std::mutex writeLock;
};

class FileClient {
   private:
    std::vector<int> OpenedFiles;
    LastRequest PreprocessReq(std::vector<char *> Request);
    int PreFilter(int fd);
    // file access control
    static std::map<int, std::string> FdToName;
    static std::map<std::string, int> NameToFd;
    static std::map<std::string, AccessCtl> FileAccess;

   public:
    static bool bIsDebugging;
    int FOPEN(std::vector<char *> Request, int &outPreFd);
    int FSEEK(std::vector<char *> Request);
    int FREAD(std::vector<char *> Request, std::string &outMessage);
    int FWRITE(std::vector<char *> Request);
    int FCLOSE(std::vector<char *> Request);
    ~FileClient();
};
