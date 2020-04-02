#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "../Lib.hpp"
#include "../ServerUtils.hpp"

class ThreadsMan {
   private:
    // save slave socket reference for the quiting process.
    static std::vector<int> SSocksRef;
    static std::mutex SSocksLock;

    static std::atomic<bool> bIsServerQuiting;

    static std::atomic<int> ThreadsCount;
    static std::atomic<int> ActiveThreads;
    static std::atomic<int> QuitingThreads;
    // static std::mutex QuitingLock;
    static bool tryQuitting();

   public:
    static int T_incr;

    static std::vector<std::thread> ThreadStash;
    static std::condition_variable NeedMoreThreads;

    static void addSScokRef(int);
    static void removeSScokRef(int);
    static void closeAllSSocks();

    static void StartQuiting() { bIsServerQuiting = true; }
    static void StopQuiting() { bIsServerQuiting = false; }
    static void RestCounter() {
        ThreadsCount = 0;
        ActiveThreads = 0;
    };

    static void ThreadCreated();
    static int getThreadsCount() { return ThreadsCount; }

    static void setActiveAndNotify();
    static void notActive();
    static int getActiveThreads() { return ActiveThreads; }
    /**
     * Thread initialize with this function,
     * invokes other functions
     * */
    static void ForeRunner(ServerSockets ServSockets,
                           std::function<void(const int)> ShellCallback,
                           std::function<void(const int)> FileCallback);
};