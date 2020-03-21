#include "ShellClient.hpp"

std::vector<std::string> ShellClient::FileNamePrefixs{"/usr/bin/", "/usr/"};

ShellClient::ShellClient() { return; }

ShellResponse ShellClient::RunShellCommand(
    std::vector<char *> &RequestTokenized) {
    // initialize default status
    ShellResponse ReportStr{"DEFAULT", -1, "DEFAULT"};

    // preping the commands
    std::string sUserCommand(RequestTokenized.at(0));
    char **aArgsPtr = RequestTokenized.data();
    /*
     !!DANGOUS!!
     It works since execve need the null terminator to function.
     I am only doing it because so far it seemed this location always
     contains garbage data.
     */
    aArgsPtr[RequestTokenized.size()] = 0;

    int ExecStat{0};

    // preping the pipe
    int TheLine[2]{-1};
    if (pipe(TheLine) == -1) {
        // pipe creation error
        throw "Error Creating the pipe";
    }

    // FORKING
    int iPId = fork();
    if (iPId == 0) {
        // child process

        close(TheLine[0]);
        dup2(TheLine[1], STDOUT_FILENO);
        dup2(TheLine[1], STDERR_FILENO);
        close(TheLine[1]);

        // try to run as-is
        execve(sUserCommand.c_str(), aArgsPtr, environ);
        // run with prefix
        for (auto Prefix : FileNamePrefixs) {
            std::string command = Prefix + sUserCommand;
            execve(command.c_str(), aArgsPtr, environ);
        }

        Utils::buoy("execve - program cannot be found");
        exit(0);
    } else {
        // parent process

        // Read the output from the child
        char ReadBuffer[4096 + 1];
        memset(ReadBuffer, 0, 4096);

        close(TheLine[1]);
        while (read(TheLine[0], ReadBuffer, sizeof(ReadBuffer)) != 0) {
            // record the output.
            std::string sOutput(ReadBuffer);
            std::cout << sOutput;
            memset(ReadBuffer, 0, 4096);
        }

        waitpid(iPId, &ExecStat, 0);
        // Child Return status.
        ExecStat = WEXITSTATUS(ExecStat);
    }

    return ReportStr;
}

ShellClient::~ShellClient() { return; }