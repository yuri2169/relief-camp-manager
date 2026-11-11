#include "TestRunner.h"
#include "Autosaver.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>
#include <stdexcept>

void run_autosave_tests() {
    testing::beginSuite("Autosaver background thread");

    // --- it actually saves, repeatedly, on its own ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        Autosaver saver;
        ASSERT_FALSE(saver.isRunning());
        ASSERT_EQ(saver.getSaveCount(), 0);

        saver.start(1, dataMutex, [&saves]() { saves++; });
        ASSERT_TRUE(saver.isRunning());

        // Wait for at least two ticks of a one-second interval.
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        saver.stop();
        ASSERT_FALSE(saver.isRunning());

        ASSERT_TRUE(saves.load() >= 2);
        ASSERT_EQ(saver.getSaveCount(), saves.load());
    }

    // --- stop() returns promptly, rather than waiting out the interval ---
    {
        std::mutex dataMutex;
        Autosaver saver;
        saver.start(3600, dataMutex, []() {});       // an hour between saves
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        saver.stop();                                 // must not block for an hour
        long elapsedMs = static_cast<long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count());
        ASSERT_TRUE(elapsedMs < 1000);
        ASSERT_FALSE(saver.isRunning());
    }

    // --- the save holds the data mutex, so it cannot see a half-written state ---
    {
        std::mutex dataMutex;
        std::atomic<bool> sawLock(false);
        std::atomic<int> saves(0);
        Autosaver saver;

        saver.start(1, dataMutex, [&]() {
            // If the saver did not already hold dataMutex, try_lock would
            // succeed here, which would mean saves can race the menu.
            if (!dataMutex.try_lock()) {
                sawLock = true;
            } else {
                dataMutex.unlock();
            }
            saves++;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        saver.stop();
        ASSERT_TRUE(saves.load() >= 1);
        ASSERT_TRUE(sawLock.load());
    }

    // --- the destructor stops the thread even if stop() was never called ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        {
            Autosaver saver;
            saver.start(1, dataMutex, [&saves]() { saves++; });
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            // no stop() - the destructor must clean up and join
        }
        int countAtDestruction = saves.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        // Nothing more happened after destruction, so the thread really joined.
        ASSERT_EQ(saves.load(), countAtDestruction);
        ASSERT_TRUE(countAtDestruction >= 1);
    }

    // --- stop() twice, and stop() without start(), are both harmless ---
    {
        std::mutex dataMutex;
        Autosaver saver;
        saver.stop();                                 // never started
        ASSERT_FALSE(saver.isRunning());
        saver.start(1, dataMutex, []() {});
        saver.stop();
        saver.stop();                                 // again
        ASSERT_FALSE(saver.isRunning());
    }

    // --- saveNow() runs the callback on the caller's thread, for menu "save" ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        Autosaver saver;
        saver.start(3600, dataMutex, [&saves]() { saves++; });
        saver.saveNow();
        ASSERT_EQ(saves.load(), 1);
        ASSERT_EQ(saver.getSaveCount(), 1);
        saver.stop();
    }

    // --- an exception thrown by the save callback does not kill the thread ---
    {
        std::mutex dataMutex;
        std::atomic<int> attempts(0);
        Autosaver saver;
        saver.start(1, dataMutex, [&attempts]() {
            attempts++;
            throw std::runtime_error("disk full");
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        ASSERT_TRUE(attempts.load() >= 2);            // it kept trying
        ASSERT_TRUE(saver.isRunning());
        saver.stop();
        ASSERT_TRUE(saver.getFailureCount() >= 2);    // and it counted the failures
    }
}
