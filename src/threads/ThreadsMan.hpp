#pragma once

#include <functional>
#include <string>
#include <thread>

#include "../ServerUtils.hpp"
#include "../Lib.hpp"

class ThreadsMan {
   private:
    /* data */
   public:
    /**
     * Fuction server as the bcackground thread function,
     * invokes other functions
     * */
    static void ThreadManager(ServerSockets &ServSockets,
                              std::function<void(const int)> ShellCallback,
                              std::function<void(const int)> FileCallback);
};