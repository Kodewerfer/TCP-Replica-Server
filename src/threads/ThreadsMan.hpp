#pragma once

#include <functional>
#include <string>
#include <thread>

#include "../Lib.hpp"
#include "../ServerUtils.hpp"

class ThreadsMan {
   private:
    /* data */
   public:
    /**
     * Total number of threads, active or not
     * */
    static int ThreadsCount;
    /**
     * Fuction server as the bcackground thread function,
     * invokes other functions
     * */
    static void ThreadManager(ServerSockets ServSockets,
                              std::function<void(const int)> ShellCallback,
                              std::function<void(const int)> FileCallback);
};