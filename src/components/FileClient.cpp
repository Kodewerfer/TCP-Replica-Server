#include "FileClient.hpp"

LastRequest FileClient::PreprocessReq(std::vector<char *> Request) {
    char *id{Request.at(1)};
    int fd = atoi(id);

    char *param{Request.at(2)};
    int params = atoi(param);

    return {fd, params};
}

int FileClient::PreFilter(int fd) {
    // filtering
    if (OpenedFiles.size() == 0) {
        return NO_PRE_OEPN;
    }
    bool bFound{false};
    for (auto ele : OpenedFiles) {
        if (fd == ele) bFound = true;
    }
    if (!bFound) return NO_SUCH_FILE;
    return 0;
}

int FileClient::FOPEN(std::vector<char *> Request) {
    if (Request.size() < 2) {
        return PARAM_PARS;
    }

    const int iFd = open(Request.at(1), O_RDWR | (O_APPEND | O_CREAT), S_IRWXU);

    if (iFd < 0) {
        throw std::string("ER-F-OP");
    }

    OpenedFiles.push_back(iFd);
    return iFd;
}

int FileClient::FSEEK(std::vector<char *> Request) {
    int res{0};
    if (Request.size() < 3) {
        return PARAM_PARS;
    }
    LastRequest Req{PreprocessReq(Request)};
    if (Req.params <= 0) {
        return PARAM_PARS;
    }
    int FilterFlag{PreFilter(Req.fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }
    Utils::rowdy("Thread " + Utils::GetTID() + " is seeking.");
    //
    if (lseek(Req.fd, Req.params, SEEK_CUR) <= 0) {
        res = -1;
    }
    Utils::rowdy("Thread " + Utils::GetTID() + " ends seeking.");

    return res;
}

int FileClient::FREAD(std::vector<char *> Request, std::string &outMessage) {
    int res{0};
    if (Request.size() < 3) {
        return PARAM_PARS;
    }
    LastRequest Req{PreprocessReq(Request)};
    if (Req.params <= 0) {
        return PARAM_PARS;
    }
    int FilterFlag{PreFilter(Req.fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    //
    char buff[Req.params + 1]{0};

    //
    Utils::rowdy("Thread " + Utils::GetTID() + " is reading.");
    if ((res = read(Req.fd, buff, Req.params)) < 0) {
        res = -1;
    }
    Utils::rowdy("Thread " + Utils::GetTID() + " ends seeking.");

    if (res == 0) {
        outMessage = "END-OF-FILE";
        return res;
    }

    //
    std::string tmp(buff);
    outMessage = tmp;

    return res;
}

int FileClient::FWRITE(std::vector<char *> Request) {
    int res{0};
    if (Request.size() < 3) {
        return PARAM_PARS;
    }

    char *id{Request.at(1)};
    int fd = atoi(id);

    int FilterFlag{PreFilter(fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }
    //
    Utils::rowdy("Thread " + Utils::GetTID() + " is writing.");
    res = write(fd, Request.at(2), strlen(Request.at(2)));
    Utils::rowdy("Thread " + Utils::GetTID() + " ends seeking.");
    if (res <= 0) {
        // failed
        return WRT_FAILED;
    }
    //

    return res;
}

int FileClient::FCLOSE(std::vector<char *> Request) {
    int res{0};
    if (Request.size() < 2) {
        return PARAM_PARS;
    }

    char *id{Request.at(1)};
    int fd = atoi(id);

    int FilterFlag{PreFilter(fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    close(fd);
    int index{0};
    for (auto ele : OpenedFiles) {
        if (ele == fd) {
            break;
        }
        index++;
    }
    OpenedFiles.erase(OpenedFiles.begin() + index);

    return res;
}

FileClient::~FileClient() {
    // close all fd.
    if (OpenedFiles.size() > 0) {
        for (auto ele : OpenedFiles) {
            close(ele);
        }
    }
}