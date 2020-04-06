#pragma once

#include <exception>
#include <string>

class FileException : public std::exception {
   private:
    std::string Code;

   public:
    FileException(std::string c) : Code(c) {}
    const char* what() const noexcept { return Code.c_str(); }
};

class ShellException : public std::exception {
   private:
    std::string Code;

   public:
    ShellException(std::string c) : Code(c) {}
    const char* what() const noexcept { return Code.c_str(); }
};

class ServerException : public std::exception {
   private:
    std::string Message;

   public:
    ServerException(std::string c) : Message(c) {}
    const char* what() const noexcept { return Message.c_str(); }
};

class AcceptingException : public std::exception {
   private:
    std::string Message;

   public:
    AcceptingException(std::string c) : Message(c) {}
    const char* what() const noexcept { return Message.c_str(); }
};

class SyncException : public std::exception {
   private:
    std::string Code;

   public:
    SyncException(std::string c) : Code(c) {}
    const char* what() const noexcept { return Code.c_str(); }
};