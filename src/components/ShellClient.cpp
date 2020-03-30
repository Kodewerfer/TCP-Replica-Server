#include "ShellClient.hpp"

std::vector<std::string> ShellClient::FileNamePrefixs{"/usr/bin/", "/usr/"};

ShellClient::ShellClient() : sLastOutput{"MARK-NO-COMMAND"} {}

int ShellClient::RunShellCommand(std::vector<char *> &RequestTokenized) {
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
        throw std::string("ERPIP");
    }

    // FORKING
    int iPId = fork();
    if (iPId == 0) {
        // child process

        /* Pipe the console outputs to the parent to store. */
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

        ServerUtils::buoy("execve - program cannot be found");
        exit(-2);
    } else {
        // parent process

        // reset the last output to blank first
        ResetLastOutput();

        // Read the output from the child
        char ReadBuffer[1024 + 1];
        memset(ReadBuffer, 0, 1024);

        close(TheLine[1]);
        while (read(TheLine[0], ReadBuffer, sizeof(ReadBuffer)) != 0) {
            // record the output to last output.
            std::string sOutput(ReadBuffer);
            sLastOutput = sOutput;
        }

        waitpid(iPId, &ExecStat, 0);
    }

    return WEXITSTATUS(ExecStat);
}