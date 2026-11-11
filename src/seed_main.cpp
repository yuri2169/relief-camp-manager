#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "Resident.h"
#include "ResidentVolunteer.h"
#include "Volunteer.h"
#include "Allocation.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "Date.h"
#include "Item.h"
#include "Symptom.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

// Builds the Morigaon demo dataset and writes it into data/ (or the directory
// given as the first argument). Everything is fixed, not random, so the demo
// behaves identically every time.
//
// The figures the demo depends on are rigged through QUANTITIES only. A camp's
// daily demand is never typed in: stock.csv stores quantities, and the loader
// recomputes demand from the families. So this program asks CsvStore for the
// same demand the loader will compute and sets quantity = days * demand.

static const int FAMILY_COUNT = 40;

// Heads of household. HEAD_SEX says whether each name is male (M) or female (F).
static const char* HEAD_NAMES[FAMILY_COUNT] = {
    "Bipul Das",        "Jiten Saikia",     "Rupa Nath",        "Hemanta Bora",
    "Gitali Pegu",      "Dhruba Kalita",    "Manju Deka",       "Ratul Medhi",
    "Anima Borah",      "Nripen Hazarika",  "Pranita Gogoi",    "Kamal Chutia",
    "Junu Sarma",       "Biren Mili",       "Mina Das",         "Probin Barman",
    "Dipti Rajbongshi", "Tarun Saikia",     "Lakhi Nath",       "Mrinal Baruah",
    "Bina Kalita",      "Haren Bora",       "Sewali Deka",      "Utpal Pegu",
    "Karuna Medhi",     "Gopal Hazarika",   "Nirmala Gogoi",    "Jatin Chutia",
    "Rekha Sarma",      "Bhaben Mili",      "Anita Barman",     "Dulal Das",
    "Padma Saikia",     "Rajen Nath",       "Maya Bora",        "Surya Kalita",
    "Parbati Deka",     "Ananta Medhi",     "Sabita Borah",     "Kuldeep Hazarika"
};
static const char HEAD_SEX[FAMILY_COUNT + 1] = "MMFMFMFMFMFMFMFMFMFMFMFMFMFMFMFMFMFMFMFM";

static const char* VILLAGES[8] = {
    "Bhuragaon", "Laharighat", "Mayong", "Mikirbheta", "Jagiroad",
    "Dharamtul", "Moirabari", "Bhurbandha"
};

// First names for the other members of a household.
static const char* WIFE_NAMES[8]    = {"Moina", "Jonaki", "Pori", "Bhanu", "Ranju", "Lipika", "Dolly", "Nayana"};
static const char* HUSBAND_NAMES[8] = {"Mukul", "Dilip", "Naren", "Pankaj", "Robin", "Sanjib", "Tapan", "Bhupen"};
static const char* CHILD_NAMES[8]   = {"Rimjim", "Jintu", "Dipjyoti", "Mamoni", "Kaushik", "Priyanka", "Bikash", "Riya"};
static const char* INFANT_NAMES[4]  = {"Aarav", "Ankita", "Dev", "Sneha"};

// The outside volunteers, one per partner organisation.
struct VolunteerRow {
    const char* name;
    int age;
    char sex;
    const char* skill;
    const char* shift;
    const char* organisation;
};
static const VolunteerRow OUTSIDE_VOLUNTEERS[4] = {
    {"Anjan Barua",      38, 'M', "medic",         "morning", "District Health"},
    {"Nirmali Gogoi",    29, 'F', "nurse",         "evening", "Red Cross"},
    {"Dilip Kalita",     45, 'M', "cook",          "morning", "ASDMA"},
    {"Pranjal Hazarika", 27, 'M', "record keeper", "morning", "Local NGO"}
};

// Stock is set as "days of supply x daily demand". One row of day counts per camp.
// Rice is the first item, the others follow in enum order.
static const double RICE_DAYS[3]  = {12.0, 2.0, 18.0};
static const double OTHER_DAYS[3] = {12.0, 8.0, 9.0};
static const double FLAT_STOCK = 40.0;      // for an item nobody draws daily (tarpaulin)

static std::string surnameOf(const std::string& fullName) {
    std::string::size_type space = fullName.rfind(' ');
    if (space == std::string::npos) return fullName;
    return fullName.substr(space + 1);
}

static bool oneOf(int oneBased, int a, int b, int c, int d, int e) {
    return oneBased == a || oneBased == b || oneBased == c || oneBased == d || oneBased == e;
}

static Date checkedDate(int y, int m, int d) {
    Date date(y, m, d);
    if (!date.isValid()) {
        throw std::logic_error("seed date is not valid: " + date.toString());
    }
    return date;
}

// Builds family number i (zero-based) following the fixed composition cycle.
static std::unique_ptr<Family> buildFamily(int i, int campId) {
    int n = i + 1;                                   // the brief numbers families from one
    std::string head = HEAD_NAMES[i];
    std::string surname = surnameOf(head);

    bool isPregnant  = oneOf(n, 4, 11, 19, 27, 35);
    bool isDisabled  = (n == 7 || n == 16 || n == 29);
    bool isLactating = (n == 9 || n == 22);
    bool hasResidentVolunteer = (n == 5 || n == 14 || n == 23 || n == 31);

    std::unique_ptr<Family> f(new Family(head, VILLAGES[i % 8]));
    int fid = f->getId();
    f->setCampId(campId);

    // First adult: the head of household.
    char headSex = HEAD_SEX[i];
    Adult* first = new Adult(head, 30 + (i % 15), headSex, fid, campId);
    first->setFlags(false, isDisabled, false);
    f->addMember(first);

    // Second adult: always present for a pregnant or lactating mother and for a
    // resident volunteer, otherwise in every second family.
    bool secondAdult = (i % 2 == 1) || isPregnant || isLactating || hasResidentVolunteer;
    if (secondAdult) {
        bool needsMother = isPregnant || isLactating;
        char sex = needsMother ? 'F' : (headSex == 'M' ? 'F' : 'M');
        std::string first_name = (sex == 'F') ? WIFE_NAMES[i % 8] : HUSBAND_NAMES[i % 8];
        std::string name = first_name + " " + surname;
        int age = 26 + (i % 14);
        if (hasResidentVolunteer) {
            // A resident volunteer is a family member who also works a shift.
            // She or he is owned by the Family, never by Registry<Volunteer>.
            std::string skill = (n % 2 == 1) ? "cook" : "record keeper";
            std::string shift = (n % 3 == 0) ? "evening" : "morning";
            f->addMember(new ResidentVolunteer(name, age, sex, fid, campId, skill, shift));
        } else {
            Adult* second = new Adult(name, age, sex, fid, campId);
            second->setFlags(isPregnant, false, isLactating);
            f->addMember(second);
        }
    }

    // Every third family has an infant; a lactating mother always does.
    if (i % 3 == 0 || isLactating) {
        std::string name = std::string(INFANT_NAMES[i % 4]) + " " + surname;
        f->addMember(new Infant(name, i % 2, (i % 2 == 0) ? 'M' : 'F', fid, campId));
    }

    // Every second family has one child, and every tenth a second one.
    if (i % 2 == 0) {
        std::string name = std::string(CHILD_NAMES[i % 8]) + " " + surname;
        f->addMember(new Child(name, 5 + (i % 7), (i % 4 == 0) ? 'M' : 'F', fid, campId));
        if (i % 5 == 0) {
            std::string name2 = std::string(CHILD_NAMES[(i + 3) % 8]) + " " + surname;
            f->addMember(new Child(name2, 3 + (i % 5), 'F', fid, campId));
        }
    }

    // Every fourth family has an elderly member.
    if (i % 4 == 0) {
        char sex = (i % 8 == 0) ? 'M' : 'F';
        std::string name = std::string(sex == 'M' ? HUSBAND_NAMES[(i + 5) % 8] : WIFE_NAMES[(i + 5) % 8]) + " " + surname;
        f->addMember(new Elderly(name, 62 + (i % 15), sex, fid, campId));
    }
    return f;
}

// Places a whole family in one shelter and records the placement on every
// resident. Camp::occupancy, the shelter's occupancy and each resident's
// shelterLabel all describe the same people, so they are updated together.
static void placeFamily(Camp& camp, Family& f) {
    int people = f.size();
    int shelterIndex = camp.findShelterFor(people);
    if (shelterIndex < 0) {
        throw std::logic_error("no shelter in " + camp.getName() + " can hold family " + f.getHeadName());
    }
    camp.admit(f);
    camp.shelterAt(shelterIndex).occupy(people);
    const std::string label = camp.shelterAt(shelterIndex).getLabel();
    for (int m = 0; m < f.size(); m++) {
        Resident* r = dynamic_cast<Resident*>(&f.memberAt(m));
        if (r != 0) {
            r->assignTo(camp.getId(), label);
        }
    }
}

int main(int argc, char** argv) {
    std::string dir = (argc > 1) ? argv[1] : "data";

    try {
        Person::resetIdCounter(1);
        Family::resetIdCounter(1);
        Camp::resetIdCounter(1);

        Registry<Camp> camps;
        Registry<Family> families;
        Registry<Volunteer> volunteers;
        DistributionLog log;
        StockLedger ledger;
        HealthMonitor health;

        // 1. The three camps, in id order 1, 2, 3.
        Camp* camp1 = camps.add(std::unique_ptr<Camp>(new Camp("Bhuragaon HS School", "Morigaon", 180)));
        camp1->addShelter(Camp::Shelter("Room-1", 50));
        camp1->addShelter(Camp::Shelter("Room-2", 50));
        camp1->addShelter(Camp::Shelter("Hall-A", 80));

        Camp* camp2 = camps.add(std::unique_ptr<Camp>(new Camp("Laharighat Community Hall", "Morigaon", 90)));
        camp2->addShelter(Camp::Shelter("Hall-B", 60));
        camp2->addShelter(Camp::Shelter("Tent-1", 30));

        Camp* camp3 = camps.add(std::unique_ptr<Camp>(new Camp("Mayong Embankment Tents", "Morigaon", 120)));
        camp3->addShelter(Camp::Shelter("Tent-A", 40));
        camp3->addShelter(Camp::Shelter("Tent-B", 40));
        camp3->addShelter(Camp::Shelter("Tent-C", 40));

        // 2-3. Forty families: 16 at camp 1, 14 at camp 2, 10 at camp 3.
        for (int i = 0; i < FAMILY_COUNT; i++) {
            Camp* camp = camp1;
            if (i >= 30) {
                camp = camp3;
            } else if (i >= 16) {
                camp = camp2;
            }
            std::unique_ptr<Family> f = buildFamily(i, camp->getId());
            placeFamily(*camp, *f);
            families.add(std::move(f));
        }

        // 4. Top camp 2 up to within 2 of capacity with unregistered walk-ins.
        // They are real people in real shelter space, so the shelter counters
        // move together with the camp's head-count.
        int walkIns = 0;
        while (camp2->freeSpace() > 2) {
            int shelterIndex = camp2->findShelterFor(1);
            if (shelterIndex < 0) break;
            ++(*camp2);
            camp2->shelterAt(shelterIndex).occupy(1);
            walkIns++;
        }
        std::cout << "Camp 2 also holds " << walkIns
                  << " unregistered walk-ins awaiting registration.\n";

        // 5. The four outside volunteers.
        for (int v = 0; v < 4; v++) {
            const VolunteerRow& row = OUTSIDE_VOLUNTEERS[v];
            volunteers.add(std::unique_ptr<Volunteer>(new Volunteer(
                row.name, row.age, row.sex, row.skill, row.shift, row.organisation)));
        }

        // 6. Stock QUANTITIES. Ask CsvStore for the demand the loader will
        // derive, then set quantity = days x demand for each item.
        CsvStore::deriveDailyDemand(camps, families);
        Date allocationDay = checkedDate(2026, 7, 12);
        Date lastRound = checkedDate(2026, 7, 13);
        Date demoDay = checkedDate(2026, 7, 14);
        for (int c = 0; c < camps.size(); c++) {
            Camp* camp = camps.at(c);
            const Ration demand = camp->getStock().getDailyDemand();
            for (int k = 0; k < ITEM_COUNT; k++) {
                Item item = itemAt(k);
                double perDay = demand.get(item);
                double quantity = FLAT_STOCK;
                if (perDay > 0.0) {
                    double days = (item == Item::Rice) ? RICE_DAYS[c] : OTHER_DAYS[c];
                    quantity = days * perDay;
                }
                camp->stockRef().set(item, quantity);
                // 9. One ledger line per camp per item. loadTxn records the
                // line without touching the stock set above.
                ledger.loadTxn(StockTxn(camp->getId(), item, quantity, true,
                                        allocationDay, "initial district allocation"));
            }
        }

        // 7. A full round at camps 1 and 3 on the day before the demo.
        // Nothing at all is recorded for the demo date.
        for (int i = 0; i < families.size(); i++) {
            const Family* f = families.at(i);
            if (f->getCampId() == 1 || f->getCampId() == 3) {
                log.loadEntry(Allocation(f->getId(), f->getCampId(), lastRound, f->dailyRation()));
            }
        }

        // 8. Health register. Camp 2 sits at 7 diarrhoea cases, below 10.
        health.logCase(2, allocationDay, Symptom::Diarrhoea, 3);
        health.logCase(2, lastRound, Symptom::Diarrhoea, 4);
        health.logCase(1, lastRound, Symptom::Fever, 2);
        health.logCase(3, lastRound, Symptom::SkinInfection, 1);

        // 10. Save through the real store and print the summary.
        CsvStore store(dir);
        store.saveAll(camps, families, volunteers, log, ledger, health);

        std::cout << "\nSeeded " << store.getDir() << ":\n"
                  << "  camps      : " << camps.size() << "\n"
                  << "  families   : " << families.size() << "\n"
                  << "  volunteers : " << volunteers.size() << "\n\n"
                  << "Demo-critical values:\n"
                  << "  camp 2 free space    : " << camp2->freeSpace()
                  << "   (demo step 3 needs <= 2)\n"
                  << "  camp 2 rice days     : " << camp2->getStock().daysOfSupplyFor(Item::Rice)
                  << "   (demo step 5 needs < 3)\n"
                  << "  camp 3 rice days     : " << camp3->getStock().daysOfSupplyFor(Item::Rice)
                  << "   (demo step 5 needs > 7)\n"
                  << "  camp 2 diarrhoea/3d  : "
                  << health.casesInWindow(2, demoDay, Symptom::Diarrhoea)
                  << "   (demo step 6 needs < 10)\n";
    } catch (const std::exception& e) {
        std::cerr << "Seed failed: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
