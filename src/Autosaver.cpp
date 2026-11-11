#include "Autosaver.h"
#include "Debug.h"
#include <chrono>

Autosaver::Autosaver()
    : dataMutex(nullptr), running(false), saveCount(0), failureCount(0), intervalSeconds(30) {}

Autosaver::~Autosaver() {
    try {
        stop();    // never leave a running thread behind
    } catch (...) {
        // a destructor must not throw
    }
}

void Autosaver::start(int seconds, std::mutex& mutexToShare, std::function<void()> fn) {
    if (running.load()) { return; }
    intervalSeconds = seconds > 0 ? seconds : 1;
    dataMutex = &mutexToShare;
    saveFn = fn;
    running = true;
    try {
        worker = std::thread(&Autosaver::loop, this);
    } catch (...) {
        running = false;          // no thread was started, so do not claim one
        throw;
    }
}

void Autosaver::loop() {
    while (running.load()) {
        // Wait for the interval, but wake immediately if stop() fires. The
        // predicate also protects against a spurious wake-up.
        {
            std::unique_lock<std::mutex> waitLock(waitMutex);
            wakeUp.wait_for(waitLock, std::chrono::seconds(intervalSeconds),
                            [this]() { return !running.load(); });
        }
        if (!running.load()) { break; }
        saveNow();
    }
    RELIEF_LIFECYCLE("autosave thread exiting after " << saveCount.load() << " saves");
}

void Autosaver::saveNow() {
    if (!saveFn) { return; }
    // Hold the data lock for the whole save, so the menu cannot change the
    // registries halfway through writing them out.
    std::lock_guard<std::mutex> dataLock(*dataMutex);
    try {
        saveFn();
    } catch (...) {
        // A failed save must not take the thread down with it; the next tick
        // tries again. The failure is counted so ConsoleApp can report it.
        failureCount++;
    }
    saveCount++;
}

void Autosaver::stop() {
    if (!running.load()) {
        if (worker.joinable()) { worker.join(); }
        return;
    }
    {
        // Change the flag while holding waitMutex. Otherwise the thread could
        // check the flag (still true), then we set it and notify before the
        // thread starts waiting, and the wake-up would be missed.
        std::lock_guard<std::mutex> waitLock(waitMutex);
        running = false;
    }
    wakeUp.notify_all();            // wake the thread out of wait_for at once
    if (worker.joinable()) { worker.join(); }
}

bool Autosaver::isRunning() const { return running.load(); }
int Autosaver::getSaveCount() const { return saveCount.load(); }
int Autosaver::getFailureCount() const { return failureCount.load(); }
