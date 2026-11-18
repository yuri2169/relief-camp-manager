#ifndef RELIEF_TESTRUNNER_H
#define RELIEF_TESTRUNNER_H

// A tiny assert-based test runner written by us. No external framework.
#include <iostream>
#include <sstream>
#include <string>
#include <cmath>

namespace testing {

inline int& passCount() { static int n = 0; return n; }
inline int& failCount() { static int n = 0; return n; }
inline std::string& currentSuite() { static std::string s = "?"; return s; }

inline void beginSuite(const std::string& name) {
    currentSuite() = name;
    std::cout << "\n-- " << name << " --\n";
}

inline void reportPass(const std::string& what) {
    passCount()++;
    std::cout << "  ok   " << what << "\n";
}

inline void reportFail(const std::string& what, const std::string& detail,
                       const char* file, int line) {
    failCount()++;
    std::cout << "  FAIL " << what << "\n"
              << "       " << detail << "\n"
              << "       at " << file << ":" << line << "\n";
}

inline int summary() {
    std::cout << "\n=========================================\n"
              << "  passed: " << passCount() << "   failed: " << failCount() << "\n"
              << "=========================================\n";
    return failCount() == 0 ? 0 : 1;
}

} // namespace testing

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (cond) { testing::reportPass(#cond); }                              \
        else { testing::reportFail(#cond, "expected true", __FILE__, __LINE__); } \
    } while (false)

#define ASSERT_FALSE(cond)                                                     \
    do {                                                                       \
        if (!(cond)) { testing::reportPass("!(" #cond ")"); }                   \
        else { testing::reportFail(#cond, "expected false", __FILE__, __LINE__); } \
    } while (false)

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        if ((actual) == (expected)) { testing::reportPass(#actual " == " #expected); } \
        else {                                                                 \
            std::ostringstream oss__;                                          \
            oss__ << "got " << (actual) << ", expected " << (expected);        \
            testing::reportFail(#actual " == " #expected, oss__.str(), __FILE__, __LINE__); \
        }                                                                      \
    } while (false)

#define ASSERT_STR_EQ(actual, expected)                                        \
    do {                                                                       \
        std::string a__ = (actual); std::string e__ = (expected);              \
        if (a__ == e__) { testing::reportPass(#actual " == " #expected); }      \
        else {                                                                 \
            testing::reportFail(#actual " == " #expected,                      \
                                "got \"" + a__ + "\", expected \"" + e__ + "\"", \
                                __FILE__, __LINE__);                           \
        }                                                                      \
    } while (false)

#define ASSERT_NEAR(actual, expected, eps)                                     \
    do {                                                                       \
        double a__ = (actual); double e__ = (expected);                        \
        if (std::fabs(a__ - e__) <= (eps)) { testing::reportPass(#actual " ~= " #expected); } \
        else {                                                                 \
            std::ostringstream oss__;                                          \
            oss__ << "got " << a__ << ", expected " << e__ << " +/- " << (eps);\
            testing::reportFail(#actual " ~= " #expected, oss__.str(), __FILE__, __LINE__); \
        }                                                                      \
    } while (false)

#define ASSERT_THROWS(stmt, ExceptionType)                                     \
    do {                                                                       \
        bool threw__ = false;                                                  \
        try { stmt; }                                                          \
        catch (const ExceptionType&) { threw__ = true; }                       \
        catch (...) {                                                          \
            testing::reportFail(#stmt " throws " #ExceptionType,               \
                                "threw a different exception type",            \
                                __FILE__, __LINE__);                           \
            break;                                                             \
        }                                                                      \
        if (threw__) { testing::reportPass(#stmt " throws " #ExceptionType); }  \
        else {                                                                 \
            testing::reportFail(#stmt " throws " #ExceptionType,               \
                                "no exception was thrown", __FILE__, __LINE__);\
        }                                                                      \
    } while (false)

#endif
