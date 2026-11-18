#include "TestRunner.h"
#include <sstream>
#include <stdexcept>

void run_runner_selftest() {
    testing::beginSuite("test runner self-test");

    ASSERT_TRUE(1 + 1 == 2);
    ASSERT_FALSE(1 + 1 == 3);
    ASSERT_EQ(2 + 2, 4);
    ASSERT_STR_EQ(std::string("relief"), "relief");
    ASSERT_NEAR(0.1 + 0.2, 0.3, 1e-9);
    ASSERT_THROWS(throw std::runtime_error("boom"), std::runtime_error);
}
