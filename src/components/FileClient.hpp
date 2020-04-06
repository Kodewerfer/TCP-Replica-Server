#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "../ServerUtils.hpp"
#include "ServerExceptions.hpp"

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
    std::atomic<bool> bIsClosing;
    std::atomic<int> reading{0};
    std::atomic<int> writing{0};
    std::mutex writeLock;
    std::condition_variable WaitOnRead;
    std::condition_variable WaitOnWrite;
    std::condition_variable WaitToClose;
};

class FileClient {
   private:
    // fds for all files opened by all users.
    static std::vector<int> OpenedFiles;
    // file access control
    static std::map<int, std::string> FdToName;
    static std::map<std::string, int> NameToFd;
    static std::map<std::string, AccessCtl> FILE_ACCESS;

   public:
    static bool bIsDebugging;

   private:
    static void endWritingAndNotify(AccessCtl &);
    static void endReadingAndNotify(AccessCtl &);
    // Stored FDs
    // Turn fd and param(if apply) to int.
    LastRequest PreprocessReq(std::vector<char *> Request);
    // Check if the fs is valid. if it has been opened.
    int FDChcker(int fd);

   public:
    static void CleanUp();
    /* if the file has been opened by other users, return the fd in the out
     parameter. */
    int FOPEN(std::vector<char *> Request, int &outPreviousFileFD);
    int FSEEK(std::vector<char *> Request);
    /* Return the file's content in the out parameter */
    int FREAD(std::vector<char *> Request, std::string &outRedContent,
              bool RunCheckingOnly = false);
    int FWRITE(std::vector<char *> Request);
    int FCLOSE(std::vector<char *> Request);
    int SYNCSEEK(std::vector<char *> Request);
    int SYNCREAD(std::vector<char *> Request, std::string &outRedContent);
    int SYNCWRITE(std::vector<char *> Request);
    int SYNCCLOSE(std::vector<char *> Request);
    // Helper functions
    std::string SyncRequestBuilder(std::vector<char *> Request);
    ~FileClient();
};
