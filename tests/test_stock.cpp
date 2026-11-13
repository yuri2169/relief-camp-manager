#include "TestRunner.h"
#include "Stock.h"
#include "Allocation.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <sstream>

void run_stock_tests() {
    testing::beginSuite("Stock, Allocation and days of supply");

    // --- Allocation ---
    Ration handout = RationNorms::forAdult() * 3.0;      // three adults for a day
    Allocation alloc(101, 1, Date(2026, 7, 14), handout);
    ASSERT_EQ(alloc.getFamilyId(), 101);
    ASSERT_EQ(alloc.getCampId(), 1);
    ASSERT_TRUE(alloc.getDate() == Date(2026, 7, 14));
    ASSERT_NEAR(alloc.getAmount().get(Item::Rice), 1.200, 1e-9);

    // An Allocation is usable as the Ration it carries.
    Ration asRation = alloc;
    ASSERT_NEAR(asRation.get(Item::Rice), 1.200, 1e-9);

    // operator== is "same family, same day" - the duplicate-claim rule.
    Allocation sameDay(101, 2, Date(2026, 7, 14), handout);     // different camp!
    Allocation nextDay(101, 1, Date(2026, 7, 15), handout);
    Allocation otherFamily(102, 1, Date(2026, 7, 14), handout);
    ASSERT_TRUE(alloc == sameDay);          // same family, same date, camp is irrelevant
    ASSERT_FALSE(alloc == nextDay);
    ASSERT_FALSE(alloc == otherFamily);

    // --- Stock basics ---
    Stock s;
    ASSERT_NEAR(s.get(Item::Rice), 0.0, 1e-9);

    s.set(Item::Rice, 500.0);
    s.set(Item::Dal, 80.0);
    s.set(Item::Oil, 40.0);
    s.set(Item::Salt, 15.0);
    s.set(Item::ORS, 300.0);
    s.set(Item::BabyFood, 20.0);
    s.set(Item::Tarpaulin, 60.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);
    ASSERT_NEAR(s[Item::Dal], 80.0, 1e-9);

    s.add(Item::Rice, 100.0);
    ASSERT_NEAR(s.get(Item::Rice), 600.0, 1e-9);

    s.take(Item::Rice, 100.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);

    // take() beyond what is there is an error, not a silent negative.
    ASSERT_THROWS(s.take(Item::Rice, 10000.0), InsufficientStockException);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // unchanged after the throw
    try {
        s.take(Item::Dal, 200.0);
    } catch (const InsufficientStockException& e) {
        ASSERT_TRUE(e.getItem() == Item::Dal);
        ASSERT_NEAR(e.getNeeded(), 200.0, 1e-9);
        ASSERT_NEAR(e.getAvailable(), 80.0, 1e-9);
    }

    // Copy constructor is independent.
    Stock copied(s);
    copied.set(Item::Rice, 1.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);

    // --- days of supply ---

    // With no demand recorded, days of supply is the "unlimited" sentinel.
    ASSERT_NEAR(s.daysOfSupply(), Stock::kUnlimitedDays, 1e-9);

    // 100 adults a day: rice 40 kg/day, dal 6 kg/day, oil 3 l/day, salt 1 kg/day,
    // ORS 10/day, baby food 0 (no demand), tarpaulin 0 (no demand).
    Ration demand = RationNorms::forAdult() * 100.0;
    s.setDailyDemand(demand);
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Rice), 500.0 / 40.0, 1e-9);     // 12.5
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Dal), 80.0 / 6.0, 1e-6);        // 13.33
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Oil), 40.0 / 3.0, 1e-6);        // 13.33
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Salt), 15.0, 1e-9);             // 15
    ASSERT_NEAR(s.daysOfSupplyFor(Item::ORS), 30.0, 1e-9);              // 30
    // Items with no demand never limit the camp.
    ASSERT_NEAR(s.daysOfSupplyFor(Item::BabyFood), Stock::kUnlimitedDays, 1e-9);

    // The worst item decides.
    ASSERT_NEAR(s.daysOfSupply(), 12.5, 1e-9);
    ASSERT_TRUE(s.mostCriticalItem() == Item::Rice);

    // A Stock used as a double IS its days of supply.
    double days = s;
    ASSERT_NEAR(days, 12.5, 1e-9);
    ASSERT_TRUE(s < 20.0);                   // reads naturally thanks to the conversion
    ASSERT_FALSE(s < 5.0);

    // Drain the rice and the critical item changes.
    Stock low(s);
    low.set(Item::Rice, 400.0);              // 10 days
    low.set(Item::Salt, 8.0);                // 8 days - now the worst
    ASSERT_NEAR(low.daysOfSupply(), 8.0, 1e-9);
    ASSERT_TRUE(low.mostCriticalItem() == Item::Salt);

    // --- operators on whole objects ---

    Stock arriving;
    arriving.set(Item::Rice, 250.0);
    arriving.set(Item::Dal, 20.0);

    Stock merged = s + arriving;
    ASSERT_NEAR(merged.get(Item::Rice), 750.0, 1e-9);
    ASSERT_NEAR(merged.get(Item::Dal), 100.0, 1e-9);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // operands untouched

    Stock accumulated(s);
    accumulated += arriving;
    ASSERT_NEAR(accumulated.get(Item::Rice), 750.0, 1e-9);

    // Stock - Allocation debits exactly what the family was handed.
    Stock afterHandout = s - alloc;
    ASSERT_NEAR(afterHandout.get(Item::Rice), 500.0 - 1.200, 1e-9);
    ASSERT_NEAR(afterHandout.get(Item::Dal), 80.0 - 0.180, 1e-9);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // the original is unchanged

    Stock debited(s);
    debited -= alloc;
    ASSERT_NEAR(debited.get(Item::Rice), 500.0 - 1.200, 1e-9);

    // Debiting more than the camp holds is refused.
    Stock nearlyEmpty;
    nearlyEmpty.set(Item::Rice, 0.5);
    ASSERT_THROWS(nearlyEmpty - alloc, InsufficientStockException);
    ASSERT_THROWS(nearlyEmpty -= alloc, InsufficientStockException);

    // The daily demand survives the arithmetic, so days of supply stays meaningful.
    ASSERT_NEAR(afterHandout.getDailyDemand().get(Item::Rice), 40.0, 1e-9);

    // Stream output lists the items with their units.
    std::ostringstream oss;
    oss << s;
    ASSERT_TRUE(oss.str().find("Rice") != std::string::npos);
    ASSERT_TRUE(oss.str().find("kg") != std::string::npos);
}
