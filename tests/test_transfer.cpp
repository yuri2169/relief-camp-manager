#include "TestRunner.h"
#include "StockLedger.h"
#include "TransferAdvisor.h"
#include "Camp.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

void run_transfer_tests() {
    testing::beginSuite("StockLedger and TransferAdvisor");

    Camp::resetIdCounter(1);
    Date today(2026, 7, 14);

    // --- StockLedger ---
    Camp school("Bhuragaon HS School", "Morigaon", 200);
    StockLedger ledger;
    ASSERT_EQ(ledger.size(), 0);

    ledger.stockIn(school, Item::Rice, 500.0, today, "district supply");
    ASSERT_NEAR(school.getStock().get(Item::Rice), 500.0, 1e-9);
    ASSERT_EQ(ledger.size(), 1);
    ASSERT_EQ(ledger.at(0).getCampId(), school.getId());
    ASSERT_TRUE(ledger.at(0).getItem() == Item::Rice);
    ASSERT_TRUE(ledger.at(0).isIncoming());
    ASSERT_STR_EQ(ledger.at(0).getNote(), "district supply");

    ledger.stockOut(school, Item::Rice, 120.0, today, "day 1 distribution");
    ASSERT_NEAR(school.getStock().get(Item::Rice), 380.0, 1e-9);
    ASSERT_EQ(ledger.size(), 2);
    ASSERT_FALSE(ledger.at(1).isIncoming());

    // The ledger's net must match the camp's stock, which is the whole point
    // of keeping a ledger rather than just a running total.
    ASSERT_NEAR(ledger.netFor(school.getId(), Item::Rice), 380.0, 1e-9);

    // Taking more than the camp holds is refused and nothing is logged.
    ASSERT_THROWS(ledger.stockOut(school, Item::Rice, 9999.0, today, "oops"),
                  InsufficientStockException);
    ASSERT_EQ(ledger.size(), 2);
    ASSERT_NEAR(school.getStock().get(Item::Rice), 380.0, 1e-9);

    ASSERT_EQ(static_cast<int>(ledger.forCamp(school.getId()).size()), 2);
    ASSERT_EQ(static_cast<int>(ledger.forCamp(999).size()), 0);
    ASSERT_THROWS(ledger.at(99), std::out_of_range);

    // --- TransferAdvisor ---
    TransferAdvisor advisor;                       // 3 days critical, 7 days surplus
    ASSERT_NEAR(advisor.getCriticalDays(), 3.0, 1e-9);
    ASSERT_NEAR(advisor.getSurplusDays(), 7.0, 1e-9);

    // Two camps with the same daily draw: 100 adults each.
    Ration demand = RationNorms::forAdult() * 100.0;     // rice 40 kg/day

    Camp shortCamp("Laharighat Community Hall", "Morigaon", 150);
    shortCamp.stockRef().setDailyDemand(demand);
    shortCamp.stockRef().set(Item::Rice, 60.0);          // 1.5 days - critical
    shortCamp.stockRef().set(Item::Dal, 60.0);           // 10 days  - fine
    shortCamp.stockRef().set(Item::Oil, 30.0);           // 10 days
    shortCamp.stockRef().set(Item::Salt, 10.0);          // 10 days
    shortCamp.stockRef().set(Item::ORS, 100.0);          // 10 days

    Camp richCamp("Mayong Embankment Tents", "Morigaon", 150);
    richCamp.stockRef().setDailyDemand(demand);
    richCamp.stockRef().set(Item::Rice, 800.0);          // 20 days - surplus
    richCamp.stockRef().set(Item::Dal, 60.0);            // 10 days
    richCamp.stockRef().set(Item::Oil, 30.0);
    richCamp.stockRef().set(Item::Salt, 10.0);
    richCamp.stockRef().set(Item::ORS, 100.0);

    ASSERT_NEAR(shortCamp.getStock().daysOfSupplyFor(Item::Rice), 1.5, 1e-9);
    ASSERT_NEAR(richCamp.getStock().daysOfSupplyFor(Item::Rice), 20.0, 1e-9);

    // criticalItems names what is running out at one camp.
    std::vector<Item> critical = advisor.criticalItems(shortCamp);
    ASSERT_EQ(static_cast<int>(critical.size()), 1);
    ASSERT_TRUE(critical[0] == Item::Rice);
    ASSERT_EQ(static_cast<int>(advisor.criticalItems(richCamp).size()), 0);

    // [Returning objects] suggest() hands back a vector of value objects.
    std::vector<Camp*> camps;
    camps.push_back(&shortCamp);
    camps.push_back(&richCamp);
    std::vector<TransferSuggestion> ideas = advisor.suggest(camps);

    ASSERT_EQ(static_cast<int>(ideas.size()), 1);
    ASSERT_EQ(ideas[0].getFromCampId(), richCamp.getId());
    ASSERT_EQ(ideas[0].getToCampId(), shortCamp.getId());
    ASSERT_TRUE(ideas[0].getItem() == Item::Rice);
    ASSERT_TRUE(ideas[0].getQuantity() > 0.0);
    ASSERT_NEAR(ideas[0].getToDays(), 1.5, 1e-9);
    ASSERT_NEAR(ideas[0].getFromDays(), 20.0, 1e-9);

    // The suggested amount must bring the short camp to safety without
    // dragging the donor below the surplus line.
    double moved = ideas[0].getQuantity();
    ASSERT_TRUE(moved <= 800.0 - (7.0 * 40.0));       // donor keeps 7 days
    ASSERT_TRUE((60.0 + moved) / 40.0 >= 3.0);        // receiver reaches 3 days

    // Applying the suggestion fixes the shortage, which is the real check.
    ledger.stockOut(richCamp, Item::Rice, moved, today, "transfer out");
    ledger.stockIn(shortCamp, Item::Rice, moved, today, "transfer in");
    ASSERT_TRUE(shortCamp.getStock().daysOfSupplyFor(Item::Rice) >= 3.0);
    ASSERT_TRUE(richCamp.getStock().daysOfSupplyFor(Item::Rice) >= 7.0);
    ASSERT_EQ(static_cast<int>(advisor.suggest(camps).size()), 0);   // nothing left to fix

    // No donor means no suggestion, rather than a bad one.
    Camp alsoShort("Second Short Camp", "Morigaon", 100);
    alsoShort.stockRef().setDailyDemand(demand);
    alsoShort.stockRef().set(Item::Rice, 40.0);        // 1 day
    std::vector<Camp*> bothShort;
    bothShort.push_back(&alsoShort);
    Camp stillShort("Third Short Camp", "Morigaon", 100);
    stillShort.stockRef().setDailyDemand(demand);
    stillShort.stockRef().set(Item::Rice, 40.0);
    bothShort.push_back(&stillShort);
    ASSERT_EQ(static_cast<int>(advisor.suggest(bothShort).size()), 0);

    // A single camp cannot transfer to itself.
    std::vector<Camp*> alone;
    alone.push_back(&alsoShort);
    ASSERT_EQ(static_cast<int>(advisor.suggest(alone).size()), 0);

    // An empty list is handled.
    std::vector<Camp*> nobody;
    ASSERT_EQ(static_cast<int>(advisor.suggest(nobody).size()), 0);

    // Custom thresholds are respected.
    TransferAdvisor strict(10.0, 15.0);
    ASSERT_NEAR(strict.getCriticalDays(), 10.0, 1e-9);
    ASSERT_TRUE(static_cast<int>(strict.criticalItems(shortCamp).size()) > 0);

    // The suggestion renders as a readable sentence for the console.
    std::string text = ideas[0].render();
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("Mayong Embankment Tents") != std::string::npos ||
                text.find(std::to_string(richCamp.getId())) != std::string::npos);

    ledger.clear();
    ASSERT_EQ(ledger.size(), 0);

    // The 999-days trap: daysOfSupplyFor() reports 999 for "no demand", which
    // is not the same as "huge surplus". A camp that merely has no demand for
    // rice (or is empty) must neither donate rice nor be asked to receive it.
    Camp noDemand("Unregistered Tents", "Morigaon", 50);
    noDemand.stockRef().set(Item::Rice, 5000.0);
    ASSERT_NEAR(noDemand.getStock().daysOfSupplyFor(Item::Rice), Stock::kUnlimitedDays, 1e-9);
    Camp needy("Needy Camp", "Morigaon", 100);
    needy.stockRef().setDailyDemand(demand);
    needy.stockRef().set(Item::Rice, 40.0);             // 1 day, critical
    std::vector<Camp*> trap;
    trap.push_back(&needy);
    trap.push_back(&noDemand);
    ASSERT_EQ(static_cast<int>(advisor.suggest(trap).size()), 0);
    ASSERT_EQ(static_cast<int>(advisor.criticalItems(noDemand).size()), 0);
    Camp emptyCamp("Empty Camp", "Morigaon", 50);
    ASSERT_EQ(static_cast<int>(advisor.criticalItems(emptyCamp).size()), 0);

    // The donor's spare is capped by what it can afford, not by the need.
    Camp thin("Thin Donor", "Morigaon", 100);
    thin.stockRef().setDailyDemand(demand);
    thin.stockRef().set(Item::Rice, 300.0);             // 7.5 days, spare 20 kg
    Camp dry("Dry Camp", "Morigaon", 100);
    dry.stockRef().setDailyDemand(demand);
    dry.stockRef().set(Item::Rice, 0.0);                // needs 120 kg
    std::vector<Camp*> limited;
    limited.push_back(&dry);
    limited.push_back(&thin);
    std::vector<TransferSuggestion> small = advisor.suggest(limited);
    ASSERT_EQ(static_cast<int>(small.size()), 1);
    ASSERT_TRUE(small[0].getItem() == Item::Rice);
    ASSERT_NEAR(small[0].getQuantity(), 20.0, 1e-9);
    ASSERT_EQ(small[0].getFromCampId(), thin.getId());

    // The richest donor is chosen when several qualify.
    Camp richer("Richer Donor", "Morigaon", 100);
    richer.stockRef().setDailyDemand(demand);
    richer.stockRef().set(Item::Rice, 1200.0);          // 30 days
    limited.push_back(&richer);
    std::vector<TransferSuggestion> pick = advisor.suggest(limited);
    ASSERT_EQ(static_cast<int>(pick.size()), 1);
    ASSERT_EQ(pick[0].getFromCampId(), richer.getId());
    ASSERT_NEAR(pick[0].getQuantity(), 120.0, 1e-9);

    // Rendered text carries labelled values.
    std::string line = pick[0].render();
    ASSERT_TRUE(line.find("Move 120 kg of Rice from Richer Donor (#") != std::string::npos);
    ASSERT_TRUE(line.find("30.0 days") != std::string::npos);
    ASSERT_TRUE(line.find("to Dry Camp (#") != std::string::npos);
    ASSERT_TRUE(line.find(", 0.0 days)") != std::string::npos);

    // A nonsense quantity is refused before it touches the stock.
    ASSERT_THROWS(ledger.stockIn(richCamp, Item::Rice, -5.0, today, "bad"), std::invalid_argument);
    ASSERT_THROWS(ledger.stockOut(richCamp, Item::Rice, 0.0, today, "bad"), std::invalid_argument);
    ASSERT_EQ(ledger.size(), 0);

    // Rejected quantities leave the stock and the log untouched.
    double before = richCamp.getStock().get(Item::Rice);
    double nan = std::numeric_limits<double>::quiet_NaN();
    double inf = std::numeric_limits<double>::infinity();
    ASSERT_THROWS(ledger.stockIn(richCamp, Item::Rice, nan, today, "bad"), std::invalid_argument);
    ASSERT_THROWS(ledger.stockIn(richCamp, Item::Rice, inf, today, "bad"), std::invalid_argument);
    ASSERT_THROWS(ledger.stockOut(richCamp, Item::Rice, nan, today, "bad"), std::invalid_argument);
    ASSERT_THROWS(ledger.stockOut(richCamp, Item::Rice, inf, today, "bad"), std::invalid_argument);
    ASSERT_NEAR(richCamp.getStock().get(Item::Rice), before, 1e-9);
    ASSERT_EQ(ledger.size(), 0);

    // One donor, two receivers: the donor's spare must not be promised twice.
    Camp donor("Shared Donor", "Morigaon", 100);
    donor.stockRef().setDailyDemand(demand);
    donor.stockRef().set(Item::Rice, 460.0);            // 11.5 days, spare 180 kg
    Camp dry1("Dry One", "Morigaon", 100);
    dry1.stockRef().setDailyDemand(demand);             // rice 0, needs 120 kg
    Camp dry2("Dry Two", "Morigaon", 100);
    dry2.stockRef().setDailyDemand(demand);
    std::vector<Camp*> shared;
    shared.push_back(&dry1);
    shared.push_back(&dry2);
    shared.push_back(&donor);
    std::vector<TransferSuggestion> two = advisor.suggest(shared);
    ASSERT_EQ(static_cast<int>(two.size()), 2);
    ASSERT_NEAR(two[0].getQuantity(), 120.0, 1e-9);     // first receiver in full
    ASSERT_NEAR(two[1].getQuantity(), 60.0, 1e-9);      // second gets the rest
    ASSERT_EQ(two[0].getToCampId(), dry1.getId());
    ASSERT_EQ(two[1].getToCampId(), dry2.getId());
    double total = two[0].getQuantity() + two[1].getQuantity();
    ASSERT_TRUE(total <= 460.0 - 7.0 * 40.0 + 1e-9);

    // Null entries are skipped rather than dereferenced.
    shared.push_back(nullptr);
    ASSERT_EQ(static_cast<int>(advisor.suggest(shared).size()), 2);

    Camp::resetIdCounter(1);
}
