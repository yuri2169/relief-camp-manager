#include "FamilySorter.h"
#include "Family.h"
#include <stdexcept>

// A selection sort, not std::sort with a lambda, so that the call through the
// member pointer - (families[j]->*metric)() - is the visible centre of the loop.
// With a few dozen families per camp the cost is irrelevant.
void FamilySorter::sortDescending(std::vector<Family*>& families, Metric metric) {
    int n = static_cast<int>(families.size());
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if ((families[j]->*metric)() > (families[best]->*metric)()) {
                best = j;
            }
        }
        if (best != i) {
            Family* temp = families[i];
            families[i] = families[best];
            families[best] = temp;
        }
    }
}

void FamilySorter::sortAscending(std::vector<Family*>& families, Metric metric) {
    int n = static_cast<int>(families.size());
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if ((families[j]->*metric)() < (families[best]->*metric)()) {
                best = j;
            }
        }
        if (best != i) {
            Family* temp = families[i];
            families[i] = families[best];
            families[best] = temp;
        }
    }
}

FamilySorter::Metric FamilySorter::metricByName(const std::string& name) {
    if (name == "vulnerability") { return &Family::metricVulnerability; }
    if (name == "size")          { return &Family::metricSize; }
    if (name == "age")           { return &Family::metricAverageAge; }
    if (name == "infants")       { return &Family::metricInfantCount; }
    throw std::invalid_argument("unknown metric: " + name);
}

std::string FamilySorter::metricMenu() {
    return "1. vulnerability  2. size  3. age  4. infants";
}
