#ifndef RELIEF_DISTRIBUTIONLOG_H
#define RELIEF_DISTRIBUTIONLOG_H

#include "Allocation.h"
#include "Ration.h"
#include "Date.h"
#include <vector>

// Every handout ever made, across every camp. The log is what makes the
// "one ration per family per day" rule enforceable: paper registers are kept
// per camp, so a family can collect twice by walking to the next camp, and
// this class is the fix for exactly that.
class DistributionLog {
    std::vector<Allocation> entries;
public:
    DistributionLog();

    // Records a handout, or throws DuplicateClaimException if this family has
    // already collected on this date in ANY camp.
    void record(const Allocation& a);

    bool alreadyClaimed(int familyId, const Date& date) const;

    int size() const;
    const Allocation& at(int index) const;        // throws std::out_of_range

    std::vector<Allocation> forCamp(int campId, const Date& date) const;
    std::vector<Allocation> forFamily(int familyId) const;
    Ration totalForCamp(int campId, const Date& date) const;
    int familiesServed(int campId, const Date& date) const;

    // Used by the CSV loader. The file records what already happened, so
    // re-checking it would reject the data being restored.
    void loadEntry(const Allocation& a);
    void clear();
};

#endif
