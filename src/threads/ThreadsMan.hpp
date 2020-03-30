#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "../Lib.hpp"
#include "../ServerUtils.hpp"

class ThreadsMan {
   private:
    static std::atomic<int> ThreadsCount;
    static std::atomic<int> ThreadsActive;

   public:
    static std::vector<std::thread> ThreadStash;
    static void incrThreadsCout();
    static void decrThreadsCout();
    static int getThreadsCout() { return ThreadsCount; }

    static void isActive();
    static void notActive();
    static int getActiveThreads() { return ThreadsActive; }

    /**
     * Thread initialize with this function,
     * invokes other functions
     * */
    static void ThreadManager(ServerSockets ServSockets,
                              std::function<void(const int)> ShellCallback,
                              std::function<void(const int)> FileCallback);
};