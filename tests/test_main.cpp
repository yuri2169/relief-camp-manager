#include "TestRunner.h"
#include <sstream>

void run_runner_selftest();
void run_date_tests();
void run_ration_tests();
void run_person_tests();
void run_hierarchy_tests();
void run_family_tests();
void run_registry_tests();
void run_exception_tests();

int main() {
    std::cout << "Relief Camp Manager - test suite\n";
    run_runner_selftest();
    run_date_tests();
    run_ration_tests();
    run_person_tests();
    run_hierarchy_tests();
    run_family_tests();
    run_registry_tests();
    run_exception_tests();
    return testing::summary();
}
