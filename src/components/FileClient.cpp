#include "FileClient.hpp"

// File access control
bool FileClient::bIsDebugging{false};

std::vector<int> FileClient::OpenedFiles;
std::map<int, std::string> FileClient::FdToName;
std::map<std::string, int> FileClient::NameToFd;
std::map<std::string, AccessCtl> FileClient::FILE_ACCESS;

void FileClient::endWritingAndNotify(AccessCtl &Control) {
    Control.writing -= 1;
    Control.WaitOnWrite.notify_all();
    Control.WaitToClose.notify_all();
}
void FileClient::endReadingAndNotify(AccessCtl &Control) {
    Control.reading -= 1;
    Control.WaitOnRead.notify_all();
    Control.WaitToClose.notify_all();
}

void FileClient::CleanUp() {
    // Clean up all fds
    for (const int &fd : OpenedFiles) {
        close(fd);
    }

    // erase all references.
    OpenedFiles.erase(OpenedFiles.begin(), OpenedFiles.end());
    FdToName.erase(FdToName.begin(), FdToName.end());
    NameToFd.erase(NameToFd.begin(), NameToFd.end());
    FILE_ACCESS.erase(FILE_ACCESS.begin(), FILE_ACCESS.end());
    return;
}

LastRequest FileClient::PreprocessReq(const RequestTokens &Request) {
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

int FileClient::FOPEN(const RequestTokens &Request, int &outPreviousFileFD) {
    if (Request.size() < 2) {
        return PARAM_PARS;
    }

    std::string FileName{Request.at(1)};

    // access control
    int FdRef = NameToFd[FileName];
    if (FdRef > 0) {
        // already opened.
        outPreviousFileFD = FdRef;
        return 0;
    }

    // int iFd = open(Request.at(1), O_RDWR | (O_APPEND | O_CREAT), S_IRWXU);
    int iFd = open(Request.at(1), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (iFd < 0) {
        throw FileException("ER-F-OP");
    }

    OpenedFiles.push_back(iFd);

    // access control
    // store refs
    FdToName[iFd] = FileName;
    NameToFd[FileName] = iFd;

    return iFd;
}

int FileClient::FSEEK(const RequestTokens &Request) {
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
    AccessCtl &Control = FILE_ACCESS[FileName];

    std::mutex FREADMutex;
    std::unique_lock<std::mutex> FREADLock(FREADMutex);

    // !! Conditional !!
    Control.WaitOnWrite.wait(FREADLock, [&] {
        return (Control.writing < 1 && !Control.bIsClosing);
    });
    //
    ++Control.reading;
    if (lseek(Req.fd, Req.params, SEEK_CUR) <= 0) {
        res = -1;
    }
    endReadingAndNotify(Control);
    return res;
}

int FileClient::FREAD(const RequestTokens &Request, std::string &outRedContent,
                      bool CheckOnly) {
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

    /*  For SYNCREAD Only.
        Only run the checking part of this op,
        so that the SyncRequestBuilder can operate normally.
     */

    if (CheckOnly) return 1;  // or it would be one of the negatives from above.

    // file access control
    std::string FileName = FdToName[Req.fd];
    AccessCtl &Control = FILE_ACCESS[FileName];

    std::mutex FREADMutex;
    std::unique_lock<std::mutex> FREADLock(FREADMutex);

    // !! Conditional !!
    Control.WaitOnWrite.wait(FREADLock, [&] {
        return (Control.writing < 1 && !Control.bIsClosing);
    });

    // the fd could have been changed while waiting on lock, check
    FilterFlag = FDChcker(Req.fd);
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " is reading.");

    ++Control.reading;

    if (bIsDebugging) {
        sleep(3);
    }

    // read
    char buff[Req.params + 1]{0};
    if ((res = read(Req.fd, buff, Req.params)) < 0) {
        res = -1;
    }
    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " ends reading.");

    endReadingAndNotify(Control);

    if (res == 0) {
        outRedContent = "END-OF-FILE";
        return res;
    }

    //
    std::string tmp(buff);
    outRedContent = tmp;

    return res;
}

int FileClient::FWRITE(const RequestTokens &Request) {
    int res{0};
    if (Request.size() < 3) {
        return PARAM_PARS;
    }

    char *id{Request.at(1)};
    int fd = atoi(id);

    // file access control
    std::string FileName = FdToName[fd];
    AccessCtl &Control = FILE_ACCESS[FileName];

    std::mutex FWRITEMutex;
    std::unique_lock<std::mutex> FWRITELock(FWRITEMutex);

    // !! Conditional !!
    Control.WaitOnRead.wait(FWRITELock, [&] {
        return (Control.reading < 1 && !Control.bIsClosing);
    });

    int FilterFlag{FDChcker(fd)};
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    // !! LOCKING !!
    std::lock_guard<std::mutex> WriteGuard(Control.writeLock);

    // Critial region
    ++Control.writing;

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " is writing.");

    if (bIsDebugging) {
        sleep(3);
    }

    // Write
    res = write(fd, Request.at(2), strlen(Request.at(2)));

    ServerUtils::rowdy("Thread " + ServerUtils::GetTID() + " ends writing.");

    endWritingAndNotify(Control);

    if (res <= 0) {
        // failed
        return WRT_FAILED;
    }

    return res;
}

int FileClient::FCLOSE(const RequestTokens &Request, bool checkOnly) {
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

    /*  For SYNCREAD Only.
        Only run the checking part of this op,
        so that the SyncRequestBuilder can operate normally.
     */
    if (checkOnly) return 1;

    // Access control
    std::string FileName = FdToName[fd];
    AccessCtl &Control = FILE_ACCESS[FileName];

    std::mutex FCLOSEMutex;
    std::unique_lock<std::mutex> FCLOSELock(FCLOSEMutex);

    // !! Conditional !!
    Control.WaitToClose.wait(FCLOSELock, [&] {
        return (Control.reading < 1 && Control.writing < 1);
    });

    Control.bIsClosing = true;

    FilterFlag = FDChcker(fd);
    if (FilterFlag < 0) {
        return FilterFlag;
    }

    // remove the reference first while still holding the fd.
    int index{0};
    for (auto ele : OpenedFiles) {
        if (ele == fd) {
            break;
        }
        index++;
    }

    OpenedFiles.erase(OpenedFiles.begin() + index);

    if (NameToFd[FileName] == fd) {
        NameToFd[FileName] = -1;
    }

    // remove fd ref.
    FdToName.erase(fd);
    FILE_ACCESS.erase(FileName);

    // close fd, release the resource
    close(fd);

    return res;
}

// Sync requests

int FileClient::SYNCSEEK(const RequestTokens &Request) {
    if (Request.size() < 3) {
        return -1;
    }
    // find the fd
    int &fd = NameToFd[Request.at(1)];
    // no fd found, open the file
    if (fd < 1) {
        int outTemp;
        FOPEN(Request, outTemp);
    }
    // rebuild the request
    RequestTokens NewRequest;
    NewRequest.push_back((char *)"FSEEK");
    NewRequest.push_back((char *)std::to_string(fd).c_str());
    NewRequest.push_back(Request.at(2));

    // read the data
    return FSEEK(NewRequest);
}

int FileClient::SYNCREAD(const RequestTokens &Request,
                         std::string &outRedContent) {
    if (Request.size() < 3) {
        return -1;
    }
    // find the fd
    int &fd = NameToFd[Request.at(1)];
    // no fd found, open the file
    if (fd < 1) {
        int outTemp;
        FOPEN(Request, outTemp);
    }
    // rebuild the request
    RequestTokens NewRequest;
    NewRequest.push_back((char *)"FREAD");
    NewRequest.push_back((char *)std::to_string(fd).c_str());
    NewRequest.push_back(Request.at(2));

    // read the data
    return FREAD(NewRequest, outRedContent);
}

int FileClient::SYNCWRITE(const RequestTokens &Request) {
    if (Request.size() < 3) {
        return -1;
    }
    // find the fd
    int &fd = NameToFd[Request.at(1)];
    // no fd found, open the file
    if (fd < 1) {
        int outTemp;
        FOPEN(Request, outTemp);
        // fd = NameToFd[Request.at(1)];
    }

    RequestTokens NewRequest;
    NewRequest.push_back((char *)"FWRITE");
    NewRequest.push_back((char *)std::to_string(fd).c_str());
    NewRequest.push_back(Request.at(2));

    return FWRITE(NewRequest);
}

int FileClient::SYNCCLOSE(const RequestTokens &Request) {
    if (Request.size() < 2) {
        return -1;
    }
    // find the fd
    int &fd = NameToFd[Request.at(1)];
    // no fd found, open the file
    if (fd < 1) {
        return 0;
    }

    RequestTokens NewRequest;
    NewRequest.push_back((char *)"FCLOSE");
    NewRequest.push_back((char *)std::to_string(fd).c_str());

    return FCLOSE(NewRequest);
}

std::string FileClient::SyncRequestBuilder(const RequestTokens &Request) {
    std::string sRequest{""};
    // request head
    std::string OriginalHead(Request.at(0));
    if (OriginalHead == "FSEEK") {
        sRequest += "SYNCSEEK ";
    }
    if (OriginalHead == "FWRITE") {
        sRequest += "SYNCWRITE ";
    }
    if (OriginalHead == "FREAD") {
        sRequest += "SYNCREAD ";
    }
    if (OriginalHead == "FCLOSE") {
        sRequest += "SYNCCLOSE ";
    }
    // file location
    char *id{Request.at(1)};
    int fd = atoi(id);
    sRequest += FdToName[fd] + " ";
    // content
    if (Request.size() > 2) {
        sRequest += std::string(Request.at(2));
    }
    return sRequest;
}

// close all fd. clear references.
FileClient::~FileClient() {
    // ServerUtils::rowdy("File Client Cleaning.");
    // if (OpenedFiles.size() > 0) {
    //     for (auto fd : OpenedFiles) {
    //         // remove the reference first while still holding the fd.
    //         std::string FileName = FdToName[fd];
    //         if (NameToFd[FileName] == fd) {
    //             NameToFd[FileName] = 0;
    //         }
    //         close(fd);
    //     }
    // }
}