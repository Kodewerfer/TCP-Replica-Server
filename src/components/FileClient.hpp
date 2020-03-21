#pragma once

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "FileErrCode.hpp"

struct LastRequest {
    int fd;
    int params;
};

class FileClient {
   private:
    std::vector<int> OpenedFiles;
    LastRequest PreprocessReq(std::vector<char *> Request);
    int PreFilter(int fd);

   public:
    int FOPEN(std::vector<char *> Request);
    int FSEEK(std::vector<char *> Request);
    int FREAD(std::vector<char *> Request, std::string &outMessage);
    int FWRITE(std::vector<char *> Request);
    int FCLOSE(std::vector<char *> Request);
    ~FileClient();
};
