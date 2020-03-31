#include "FileClient.hpp"

// File access control
bool FileClient::bIsDebugging{false};
std::map<int, std::string> FileClient::FdToName{};
std::map<std::string, int> FileClient::NameToFd{};
std::map<std::string, AccessCtl> FileClient::FileAccess{};

LastRequest FileClient::PreprocessReq(std::vector<char *> Request) {
    char *id{Request.at(1)};
    int fd = atoi(id);

    char *param{Request.at(2)};
    int params = atoi(param);

    return {fd, params};
}

int FileClient::FDChcker(int fd) {
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

int FileClient::FOPEN(std::vector<char *> Request, int &outPreFd) {
    if (Request.size() < 2) {
        return PARAM_PARS;
    }

    std::string FileName = std::string(Request.at(1));

    // access control
    int FdRef = NameToFd[FileName];
    if (FdRef > 0) {
        // already opened.
        outPreFd = FdRef;
        // already opened by self.
        for (auto ele : OpenedFiles) {
            if (ele == FdRef) {
                throw std::string("ER-F-FD");
            }
        }
    }

    int iFd = open(Request.at(1), O_RDWR | (O_APPEND | O_CREAT), S_IRWXU);

    if (iFd < 0) {
        throw std::string("ER-F-OP");
    }

    OpenedFiles.push_back(iFd);

    // access control
    // store refs
    FdToName[iFd] = FileName;
    NameToFd[FileName] = iFd;

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
    int FilterFlag{FDChcker(Req.fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }
    //
    if (lseek(Req.fd, Req.params, SEEK_CUR) <= 0) {
        res = -1;
    }

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
    int FilterFlag{FDChcker(Req.fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    // file access control
    std::string FileName = FdToName[Req.fd];
    AccessCtl &Control = FileAccess[FileName];

    if (Control.writing > 0) {
        throw std::string("ER-F-ACEDI");
    }

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " is reading.");
    Control.reading += 1;

    if (bIsDebugging) {
        sleep(3);
    }

    // read
    char buff[Req.params + 1]{0};
    if ((res = read(Req.fd, buff, Req.params)) < 0) {
        res = -1;
    }
    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " ends reading.");
    Control.reading -= 1;

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

    int FilterFlag{FDChcker(fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    // file access control
    std::string FileName = FdToName[fd];
    AccessCtl &Control = FileAccess[FileName];

    // !! LOCKING !!
    std::unique_lock<std::mutex> WritingLock(Control.writeLock,
                                             std::try_to_lock);
    if (Control.reading > 0 || !WritingLock.owns_lock()) {
        //  abort.
        throw std::string("ER-F-ACEDI");
    }
    // Critial region
    Control.writing += 1;

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " is writing.");

    if (bIsDebugging) {
        sleep(3);
    }
    // Write
    res = write(fd, Request.at(2), strlen(Request.at(2)));

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " ends seeking.");

    Control.writing -= 1;

    if (res <= 0) {
        // failed
        return WRT_FAILED;
    }

    return res;
}

int FileClient::FCLOSE(std::vector<char *> Request) {
    int res{0};
    if (Request.size() < 2) {
        return PARAM_PARS;
    }

    char *id{Request.at(1)};
    int fd = atoi(id);

    int FilterFlag{FDChcker(fd)};
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

    // remove fd ref.
    FdToName.erase(fd);

    return res;
}

// close all fd.
FileClient::~FileClient() {
    if (OpenedFiles.size() > 0) {
        for (auto ele : OpenedFiles) {
            close(ele);
        }
    }
}