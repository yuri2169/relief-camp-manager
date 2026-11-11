#include "TestRunner.h"
#include "DistributionLog.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <stdexcept>
#include <vector>

void run_distribution_tests() {
    testing::beginSuite("DistributionLog and duplicate claims");

    DistributionLog log;
    ASSERT_EQ(log.size(), 0);

    Ration oneAdult = RationNorms::forAdult();
    Date day1(2026, 7, 14);
    Date day2(2026, 7, 15);

    // A first claim is recorded.
    log.record(Allocation(101, 1, day1, oneAdult * 5.0));
    ASSERT_EQ(log.size(), 1);
    ASSERT_TRUE(log.alreadyClaimed(101, day1));
    ASSERT_FALSE(log.alreadyClaimed(101, day2));
    ASSERT_FALSE(log.alreadyClaimed(102, day1));

    // The same family, the same day, the SAME camp: refused.
    ASSERT_THROWS(log.record(Allocation(101, 1, day1, oneAdult)), DuplicateClaimException);
    ASSERT_EQ(log.size(), 1);               // nothing was added

    // The same family, the same day, a DIFFERENT camp: still refused. This is
    // the rule that stops a family collecting twice by walking to the next camp.
    ASSERT_THROWS(log.record(Allocation(101, 2, day1, oneAdult)), DuplicateClaimException);
    ASSERT_EQ(log.size(), 1);

    bool caught = false;
    try {
        log.record(Allocation(101, 3, day1, oneAdult));
    } catch (const DuplicateClaimException& e) {
        caught = true;
        ASSERT_EQ(e.getFamilyId(), 101);
        ASSERT_TRUE(e.getDate() == day1);
    }
    ASSERT_TRUE(caught);
    // The refused claims left the log exactly as it was: the one entry is
    // still the original camp-1 claim, and no camp 2 or camp 3 entry exists.
    ASSERT_EQ(log.size(), 1);
    ASSERT_EQ(log.at(0).getCampId(), 1);
    ASSERT_EQ(log.familiesServed(2, day1), 0);
    ASSERT_EQ(log.familiesServed(3, day1), 0);

    // The next day the same family may claim again.
    log.record(Allocation(101, 1, day2, oneAdult * 5.0));
    ASSERT_EQ(log.size(), 2);
    ASSERT_TRUE(log.alreadyClaimed(101, day2));

    // Other families are unaffected.
    log.record(Allocation(102, 1, day1, oneAdult * 3.0));
    log.record(Allocation(103, 2, day1, oneAdult * 4.0));
    ASSERT_EQ(log.size(), 4);

    // Queries by camp and day.
    std::vector<Allocation> camp1Day1 = log.forCamp(1, day1);
    ASSERT_EQ(static_cast<int>(camp1Day1.size()), 2);       // families 101 and 102
    ASSERT_EQ(log.familiesServed(1, day1), 2);
    ASSERT_EQ(log.familiesServed(2, day1), 1);
    ASSERT_EQ(log.familiesServed(9, day1), 0);

    // The camp's total for the day is the sum of its allocations.
    Ration camp1Total = log.totalForCamp(1, day1);
    ASSERT_NEAR(camp1Total.get(Item::Rice), 0.400 * 8.0, 1e-9);   // 5 + 3 adults

    Ration emptyTotal = log.totalForCamp(9, day1);
    ASSERT_TRUE(emptyTotal.isZero());

    // A family's whole history, across camps and days.
    std::vector<Allocation> history = log.forFamily(101);
    ASSERT_EQ(static_cast<int>(history.size()), 2);

    // at() is positional and range-checked.
    ASSERT_EQ(log.at(0).getFamilyId(), 101);
    ASSERT_THROWS(log.at(99), std::out_of_range);
    ASSERT_THROWS(log.at(-1), std::out_of_range);

    // loadEntry skips the duplicate check, because the CSV on disk is the
    // record of what already happened - re-validating it would reject the
    // very data we are restoring.
    DistributionLog loaded;
    loaded.loadEntry(Allocation(101, 1, day1, oneAdult));
    loaded.loadEntry(Allocation(101, 1, day1, oneAdult));    // no throw
    ASSERT_EQ(loaded.size(), 2);
    // ...but a NEW claim after loading is still checked.
    ASSERT_THROWS(loaded.record(Allocation(101, 1, day1, oneAdult)), DuplicateClaimException);
    ASSERT_EQ(loaded.size(), 2);

    log.clear();
    ASSERT_EQ(log.size(), 0);
    ASSERT_FALSE(log.alreadyClaimed(101, day1));
}
