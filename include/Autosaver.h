#ifndef RELIEF_AUTOSAVER_H
#define RELIEF_AUTOSAVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

// [Multithreading] Saves the data to disk every few seconds on a background
// thread, so an unannounced power cut - routine in a flood-hit district -
// costs at most one interval of work instead of the whole day's register.
//
// Two things make this safe:
//  * the save takes the SAME mutex the menu takes before it changes anything,
//    so a save can never catch the data half-written;
//  * stop() signals a condition_variable instead of letting the thread sleep
//    out its interval, so quitting the app is immediate rather than taking up
//    to N seconds.
class Autosaver {
    std::thread worker;
    std::mutex* dataMutex;            // borrowed from ConsoleApp, not owned
    std::function<void()> saveFn;
    std::atomic<bool> running;
    std::atomic<int> saveCount;
    std::atomic<int> failureCount;    // saves whose callback threw

    std::mutex waitMutex;             // guards the condition_variable only
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
    void stop();
    bool isRunning() const;
    int getSaveCount() const;
    int getFailureCount() const;      // lets ConsoleApp tell the operator a save failed
    void saveNow();                   // runs the save on the calling thread
};

#endif
