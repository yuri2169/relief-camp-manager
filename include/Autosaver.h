#ifndef RELIEF_AUTOSAVER_H
#define RELIEF_AUTOSAVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

// Saves the data to disk every few seconds on a background
// thread, so an unannounced power cut - routine in a flood-hit district -
// costs at most one interval of work instead of the whole day's register.
//
// Two things make this safe:
//  * the save takes the SAME mutex the menu takes before it changes anything,
//    so a save can never catch the data half-written;
//  * stop() signals a condition_variable instead of letting the thread sleep
//    out its interval, so quitting the app is immediate rather than taking up
//    to N seconds.
//
// Rules for callers:
//  * start(), stop() and the destructor are single-threaded entry points: call
//    them from one thread only, and never from the worker thread itself.
//  * call stop() and saveNow() WITHOUT holding the data mutex, because the
//    saver takes that mutex itself (std::mutex is not recursive).
class Autosaver {
    // [Multithreading]
    std::thread worker;
    // [Mutex]
    std::mutex* dataMutex;            // borrowed from ConsoleApp, not owned
    std::function<void()> saveFn;
    // [Atomic variable]
    std::atomic<bool> running;
    std::atomic<int> saveCount;
    std::atomic<int> failureCount;    // saves whose callback threw

    std::mutex waitMutex;             // guards the condition_variable only
    // [Condition variable]
    std::condition_variable wakeUp;
    int intervalSeconds;

    void loop();                      // the thread body

public:
    Autosaver();
    ~Autosaver();                     // stops and joins, even if stop() was missed

    // A running thread must not be copied, and the compiler cannot guess what
    // copying one would mean, so say it explicitly.
    Autosaver(const Autosaver& other) = delete;
    Autosaver& operator=(const Autosaver& other) = delete;

    void start(int intervalSeconds, std::mutex& dataMutex, std::function<void()> saveFn);
    // Call without holding the data mutex: the saver may be waiting for it.
    void stop();
    bool isRunning() const;
    int getSaveCount() const;         // save ATTEMPTS, including ones that threw
    int getFailureCount() const;      // lets ConsoleApp tell the operator a save failed
    // Runs the save on the calling thread. Call without holding the data mutex.
    void saveNow();
};

#endif
