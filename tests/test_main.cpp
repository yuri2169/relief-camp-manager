#include "TestRunner.h"
#include <sstream>

void run_runner_selftest();
void run_date_tests();
void run_ration_tests();

int main() {
    std::cout << "Relief Camp Manager - test suite\n";
    run_runner_selftest();
    run_date_tests();
    run_ration_tests();
    return testing::summary();
}
