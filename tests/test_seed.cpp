#include "TestRunner.h"
#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Resident.h"
#include "Volunteer.h"
#include "ResidentVolunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "TransferAdvisor.h"
#include <iostream>
#include <string>
#include <vector>

// Checks the shipped demo dataset in data/, so a broken data/ directory fails
// the test suite instead of failing in front of the professor. It only reads.

static int familiesAtCamp(const Registry<Family>& families, int campId) {
    int n = 0;
    for (int i = 0; i < families.size(); i++) {
        if (families.at(i)->getCampId() == campId) n++;
    }
    return n;
}

void run_seed_tests() {
    testing::beginSuite("Seeded Morigaon dataset");

    CsvStore store("data");
    if (!store.dataExists()) {
        std::cout << "  skip  data/ not seeded yet - run `make seed`\n";
        return;
    }

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    Registry<Camp> camps;
    Registry<Family> families;
    Registry<Volunteer> volunteers;
    DistributionLog log;
    StockLedger ledger;
    HealthMonitor health;

    store.loadAll(camps, families, volunteers, log, ledger, health);

    ASSERT_EQ(camps.size(), 3);
    ASSERT_EQ(families.size(), 40);
    ASSERT_EQ(volunteers.size(), 4);

    Date demoDay(2026, 7, 14);
    Date dayBefore(2026, 7, 13);
    ASSERT_TRUE(demoDay.isValid());
    ASSERT_TRUE(dayBefore.isValid());

    // --- camps and shelters, exactly as the brief names them ---
    const char* campNames[3] = {"Bhuragaon HS School", "Laharighat Community Hall",
                                "Mayong Embankment Tents"};
    int campCapacity[3] = {180, 90, 120};
    int shelterCount[3] = {3, 2, 3};
    const char* shelterLabels[3][3] = {{"Room-1", "Room-2", "Hall-A"},
                                       {"Hall-B", "Tent-1", ""},
                                       {"Tent-A", "Tent-B", "Tent-C"}};
    int shelterCap[3][3] = {{50, 50, 80}, {60, 30, 0}, {40, 40, 40}};
    for (int c = 0; c < 3; c++) {
        Camp* camp = camps.findById(c + 1);
        ASSERT_TRUE(camp != 0);
        if (camp == 0) continue;
        ASSERT_STR_EQ(camp->getName().c_str(), campNames[c]);
        ASSERT_STR_EQ(camp->getDistrict().c_str(), "Morigaon");
        ASSERT_EQ(camp->getCapacity(), campCapacity[c]);
        ASSERT_EQ(camp->shelterCount(), shelterCount[c]);
        int shelterTotal = 0;
        for (int s = 0; s < camp->shelterCount(); s++) {
            ASSERT_STR_EQ(camp->shelterAt(s).getLabel().c_str(), shelterLabels[c][s]);
            ASSERT_EQ(camp->shelterAt(s).getCapacity(), shelterCap[c][s]);
            shelterTotal += camp->shelterAt(s).getOccupancy();
        }
        // The occupancy contract: the placement detail adds up to the head-count.
        ASSERT_EQ(shelterTotal, camp->getOccupancy());
        ASSERT_TRUE(camp->getOccupancy() <= camp->getCapacity());
    }

    // --- 40 families: 16 / 14 / 10 ---
    ASSERT_EQ(familiesAtCamp(families, 1), 16);
    ASSERT_EQ(familiesAtCamp(families, 2), 14);
    ASSERT_EQ(familiesAtCamp(families, 3), 10);

    int infants = 0, elderly = 0, pregnant = 0, disabled = 0, lactating = 0;
    int residentVolunteers = 0;
    int campHeads[4] = {0, 0, 0, 0};     // residents found per camp
    for (int i = 0; i < families.size(); i++) {
        Family* f = families.at(i);
        ASSERT_TRUE(f->size() >= 1);
        ASSERT_TRUE(f->getCampId() >= 1 && f->getCampId() <= 3);
        infants += f->countOfTag("INFANT");
        elderly += f->countOfTag("ELDERLY");
        residentVolunteers += f->countOfTag("RESIDENT_VOLUNTEER");
        for (int m = 0; m < f->size(); m++) {
            const Person& p = f->memberAt(m);
            if (p.isPregnant()) pregnant++;
            if (p.isDisabled()) disabled++;
            if (p.isLactating()) lactating++;
            // Every resident is placed in a shelter, or the placement was lost.
            const Resident* r = dynamic_cast<const Resident*>(&p);
            ASSERT_TRUE(r != 0);
            if (r != 0) {
                ASSERT_FALSE(r->getShelterLabel().empty());
                campHeads[f->getCampId()]++;
            }
        }
    }
    ASSERT_TRUE(infants >= 10);
    ASSERT_TRUE(elderly >= 8);
    ASSERT_EQ(pregnant, 5);
    ASSERT_EQ(disabled, 3);
    ASSERT_EQ(lactating, 2);
    ASSERT_EQ(residentVolunteers, 4);
    // Every occupant is a registered family member: no hidden head-count.
    for (int c = 1; c <= 3; c++) {
        ASSERT_EQ(campHeads[c], camps.findById(c)->getOccupancy());
        // Families are spread: more than one shelter is in use.
        int used = 0;
        for (int s = 0; s < camps.findById(c)->shelterCount(); s++) {
            if (camps.findById(c)->shelterAt(s).getOccupancy() > 0) used++;
        }
        ASSERT_TRUE(used >= 2);
    }

    // --- outside volunteers; a ResidentVolunteer is never in the registry ---
    for (int i = 0; i < volunteers.size(); i++) {
        ASSERT_TRUE(dynamic_cast<ResidentVolunteer*>(volunteers.at(i)) == 0);
        ASSERT_TRUE(volunteers.at(i)->typeTag() != "RESIDENT_VOLUNTEER");
        ASSERT_FALSE(volunteers.at(i)->getOrganisation().empty());
    }
    std::vector<std::string> orgs;
    for (int i = 0; i < volunteers.size(); i++) {
        orgs.push_back(volunteers.at(i)->getOrganisation());
    }
    ASSERT_STR_EQ(orgs[0].c_str(), "District Health");
    ASSERT_STR_EQ(orgs[1].c_str(), "Red Cross");
    ASSERT_STR_EQ(orgs[2].c_str(), "ASDMA");
    ASSERT_STR_EQ(orgs[3].c_str(), "Local NGO");

    // --- the facts the demo script depends on ---
    Camp* c1 = camps.findById(1);
    Camp* c2 = camps.findById(2);
    Camp* c3 = camps.findById(3);

    // Demo step 3: camp 2 is nearly full.
    ASSERT_TRUE(c2->freeSpace() <= 2);
    ASSERT_TRUE(c2->freeSpace() >= 0);
    ASSERT_EQ(c2->getOccupancy(), 88);
    // No camp 2 family is small enough to fit in what is left, so admitting
    // one is refused. Camp 2 families are about six people each.
    for (int i = 0; i < families.size(); i++) {
        if (families.at(i)->getCampId() == 2) {
            ASSERT_TRUE(families.at(i)->size() >= 5);
            ASSERT_TRUE(families.at(i)->size() > c2->freeSpace());
        }
    }

    // Demo step 5: camp 2's rice is about 2 days and is its worst item.
    double rice2 = c2->getStock().daysOfSupplyFor(Item::Rice);
    ASSERT_TRUE(rice2 >= 1.5 && rice2 <= 3.0);
    ASSERT_TRUE(c2->getStock().mostCriticalItem() == Item::Rice);
    // Everything else at camp 2 is comfortably above the critical line.
    Item others[5] = {Item::Dal, Item::Oil, Item::Salt, Item::ORS, Item::BabyFood};
    for (int k = 0; k < 5; k++) {
        ASSERT_TRUE(c2->getStock().daysOfSupplyFor(others[k]) >= 6.0);
        ASSERT_TRUE(c1->getStock().daysOfSupplyFor(others[k]) >= 6.0);
        ASSERT_TRUE(c3->getStock().daysOfSupplyFor(others[k]) >= 6.0);
    }
    ASSERT_TRUE(c1->getStock().daysOfSupplyFor(Item::Rice) >= 10.0);

    // Camp 3 is the donor, around 18 days of rice.
    double rice3 = c3->getStock().daysOfSupplyFor(Item::Rice);
    ASSERT_TRUE(rice3 >= 16.0 && rice3 <= 20.0);

    // The advisor finds camp 3 -> camp 2 for rice.
    std::vector<Camp*> all;
    for (int i = 0; i < camps.size(); i++) all.push_back(camps.at(i));
    TransferAdvisor advisor;
    std::vector<TransferSuggestion> moves = advisor.suggest(all);
    bool riceFrom3To2 = false;
    for (std::size_t i = 0; i < moves.size(); i++) {
        if (moves[i].getItem() == Item::Rice && moves[i].getFromCampId() == 3 &&
            moves[i].getToCampId() == 2) {
            riceFrom3To2 = true;
        }
    }
    ASSERT_TRUE(riceFrom3To2);

    // Demo step 6: camp 2's diarrhoea window holds 7, just under the threshold.
    ASSERT_EQ(health.casesInWindow(2, demoDay, Symptom::Diarrhoea), 7);
    ASSERT_TRUE(HealthMonitor::thresholdFor(Symptom::Diarrhoea) > 7);
    ASSERT_FALSE(health.outbreakAlert(2, demoDay, Symptom::Diarrhoea));
    ASSERT_EQ(health.casesInWindow(1, demoDay, Symptom::Fever), 2);
    ASSERT_EQ(health.casesInWindow(3, demoDay, Symptom::SkinInfection), 1);

    // Demo step 4: a round is on record for the 13th and nothing for the 14th.
    ASSERT_TRUE(log.size() > 0);
    int onDayBefore = 0, onDemoDay = 0;
    for (int i = 0; i < log.size(); i++) {
        if (log.at(i).getDate() == dayBefore) onDayBefore++;
        if (log.at(i).getDate() == demoDay) onDemoDay++;
    }
    ASSERT_TRUE(onDayBefore > 0);
    ASSERT_EQ(onDemoDay, 0);
    for (int i = 0; i < families.size(); i++) {
        ASSERT_FALSE(log.alreadyClaimed(families.at(i)->getId(), demoDay));
    }

    // The stock register agrees with the stock on hand.
    for (int c = 1; c <= 3; c++) {
        for (int k = 0; k < ITEM_COUNT; k++) {
            Item it = itemAt(k);
            ASSERT_NEAR(ledger.netFor(c, it), camps.findById(c)->getStock().get(it), 0.001);
        }
    }

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
