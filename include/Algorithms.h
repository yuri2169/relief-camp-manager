#ifndef RELIEF_ALGORITHMS_H
#define RELIEF_ALGORITHMS_H

#include <vector>

// Finds the element scoring highest on a metric the caller chooses, where the
// metric is a member function of T. One function replaces a separate
// findMostVulnerable, findLargest, findOldest and so on.
//
// Example: findMax(families, &Family::metricVulnerability)
template <typename T>
T* findMax(const std::vector<T*>& items, double (T::*metric)() const) {
    if (items.empty()) {
        return 0;
    }
    T* best = items[0];
    double bestValue = (best->*metric)();      // call through the member pointer
    for (int i = 1; i < static_cast<int>(items.size()); i++) {
        double value = (items[i]->*metric)();
        if (value > bestValue) {
            bestValue = value;
            best = items[i];
        }
    }
    return best;
}

#endif
