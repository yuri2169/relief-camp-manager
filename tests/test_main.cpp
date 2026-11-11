#include "TestRunner.h"
#include <sstream>

void run_runner_selftest();

int main() {
    std::cout << "Relief Camp Manager - test suite\n";
    run_runner_selftest();
    return testing::summary();
}
