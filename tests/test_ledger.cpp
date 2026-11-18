#include "TestRunner.h"
#include "StockLedger.h"
#include "Camp.h"
#include <sstream>

void run_ledger_tests() {
    testing::beginSuite("StockLedger history");

    Camp::resetIdCounter(1);
    Camp a("Camp A", "Morigaon", 100);
    Camp b("Camp B", "Morigaon", 100);
    StockLedger ledger;

    ledger.stockIn(a, Item::Rice, 100.0, Date(2026, 7, 14), "first");
    ledger.stockIn(b, Item::Rice, 50.0, Date(2026, 7, 14), "other camp");
    ledger.stockIn(a, Item::Dal, 30.0, Date(2026, 7, 15), "dal");
    ledger.stockOut(a, Item::Rice, 40.0, Date(2026, 7, 16), "last");

    // History keeps insertion order.
    ASSERT_EQ(ledger.size(), 4);
    ASSERT_STR_EQ(ledger.at(0).getNote(), "first");
    ASSERT_STR_EQ(ledger.at(1).getNote(), "other camp");
    ASSERT_STR_EQ(ledger.at(2).getNote(), "dal");
    ASSERT_STR_EQ(ledger.at(3).getNote(), "last");
    ASSERT_TRUE(ledger.at(3).getDate() == Date(2026, 7, 16));
    ASSERT_NEAR(ledger.at(3).getQuantity(), 40.0, 1e-9);

    // forCamp filters by camp and keeps order.
    std::vector<StockTxn> forA = ledger.forCamp(a.getId());
    ASSERT_EQ(static_cast<int>(forA.size()), 3);
    ASSERT_STR_EQ(forA[0].getNote(), "first");
    ASSERT_STR_EQ(forA[2].getNote(), "last");

    // netFor is per camp and per item.
    ASSERT_NEAR(ledger.netFor(a.getId(), Item::Rice), 60.0, 1e-9);
    ASSERT_NEAR(ledger.netFor(a.getId(), Item::Dal), 30.0, 1e-9);
    ASSERT_NEAR(ledger.netFor(b.getId(), Item::Rice), 50.0, 1e-9);
    ASSERT_NEAR(ledger.netFor(b.getId(), Item::Dal), 0.0, 1e-9);
    ASSERT_NEAR(a.getStock().get(Item::Rice), ledger.netFor(a.getId(), Item::Rice), 1e-9);

    // loadTxn appends without touching any camp.
    ledger.loadTxn(StockTxn(a.getId(), Item::Oil, 5.0, true, Date(2026, 7, 17), "loaded"));
    ASSERT_EQ(ledger.size(), 5);
    ASSERT_NEAR(a.getStock().get(Item::Oil), 0.0, 1e-9);
    ASSERT_NEAR(ledger.netFor(a.getId(), Item::Oil), 5.0, 1e-9);

    // Printing labels direction, item and quantity.
    std::ostringstream out;
    out << ledger.at(3);
    ASSERT_TRUE(out.str().find("OUT") != std::string::npos);
    ASSERT_TRUE(out.str().find("Rice") != std::string::npos);
    ASSERT_TRUE(out.str().find("40") != std::string::npos);

    ledger.clear();
    ASSERT_EQ(ledger.size(), 0);
    Camp::resetIdCounter(1);
}
