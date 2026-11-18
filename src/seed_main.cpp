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
// Composition rules (family number n is one-based, i = n - 1):
//  - Camps: families 1-16 at camp 1, 17-30 at camp 2, 31-40 at camp 3.
//  - Every family has 1 or 2 adults. The second adult is present in every
//    second family, and always when the family needs one (see below).
//  - Every third family has an infant. The brief leaves family 9 (a lactating
//    mother) without one, so a lactating family is FORCED to have an infant.
//  - Every second family (i even) has one child; only the families with i
//    divisible by ten (family numbers 1, 11, 21, 31) have a second child, so
//    "every second family has one or two children" holds.
//  - Every fourth family has an elderly member.
//  - Pregnant (4, 11, 19, 27, 35) and lactating (9, 22) flags land on a SECOND
//    adult, who is female. That second adult is forced to exist for those
//    families. The disabled flag (7, 16, 29) is on the head of household.
//  - Families 5, 14, 23, 31 have a ResidentVolunteer as their second adult.
//    A ResidentVolunteer belongs to its Family and never to Registry<Volunteer>.
//  - Camp 2 must be nearly full from REGISTERED families alone, so its 14
//    families (about 6 people each) carry 3 or 4 extra children each. Only the
//    number of Child members grows; no other member type is changed, so the
//    age bands and ration norms stay honest. Together they hold 88 of 90, which
//    leaves 2 free: no family can be admitted.
//  - Families are spread over a camp's shelters (the emptiest shelter by
//    fraction filled that still has room), so several rooms show in use.
//
// The figures the demo depends on are rigged through QUANTITIES only. A camp's
// daily demand is never typed in: stock.csv stores quantities, and the loader
// recomputes demand from the families. So this program asks CsvStore for the
// same demand the loader will compute and sets quantity = days * demand.

static const int FAMILY_COUNT = 40;

// Camp 2 is filled by its own 14 families (family numbers 17-30, zero-based
// 16-29). Each gets 3 extra children and the first 6 of them get a fourth,
// so the camp holds 88 of 90 and no registered family fits in what is left.
static const int CAMP2_FIRST_INDEX = 16;
static const int CAMP2_BASE_EXTRA_CHILDREN = 3;
static const int CAMP2_FAMILIES_WITH_ONE_MORE = 6;

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

// First names for the other members of a household. Each name is stored with
// its sex, so a name and a sex can never disagree.
struct NamedPerson {
    const char* name;
    char sex;
};
static const NamedPerson INFANTS[4] = {
    {"Aarav", 'M'}, {"Ankita", 'F'}, {"Dev", 'M'}, {"Sneha", 'F'}
};
// Twelve children. A family takes consecutive entries (wrapping), and no
// family has more than twelve, so no two children in a family share a name.
static const NamedPerson CHILDREN[12] = {
    {"Jintu", 'M'},   {"Rimjim", 'F'},   {"Dipjyoti", 'M'}, {"Mamoni", 'F'},
    {"Kaushik", 'M'}, {"Priyanka", 'F'}, {"Bikash", 'M'},   {"Riya", 'F'},
    {"Himangshu", 'M'}, {"Juri", 'F'},   {"Rajib", 'M'},    {"Nandita", 'F'}
};
static const char* WIFE_NAMES[8]    = {"Moina", "Jonaki", "Pori", "Bhanu", "Ranju", "Lipika", "Dolly", "Nayana"};
static const char* HUSBAND_NAMES[8] = {"Mukul", "Dilip", "Naren", "Pankaj", "Robin", "Sanjib", "Tapan", "Bhupen"};

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
static std::unique_ptr<Family> buildFamily(int i, int campId, int extraChildren) {
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

    // Children take the next names from the table, starting at a different
    // place in each family.
    int childSlot = i;

    // Every third family has an infant; a lactating mother always does.
    if (i % 3 == 0 || isLactating) {
        const NamedPerson& baby = INFANTS[i % 4];
        std::string name = std::string(baby.name) + " " + surname;
        f->addMember(new Infant(name, i % 2, baby.sex, fid, campId));
    }

    // Every second family has one child, and every tenth a second one.
    if (i % 2 == 0) {
        const NamedPerson& kid = CHILDREN[childSlot % 12];
        childSlot++;
        std::string name = std::string(kid.name) + " " + surname;
        f->addMember(new Child(name, 5 + (i % 7), kid.sex, fid, campId));
        if (i % 5 == 0) {
            const NamedPerson& kid2 = CHILDREN[childSlot % 12];
            childSlot++;
            std::string name2 = std::string(kid2.name) + " " + surname;
            f->addMember(new Child(name2, 3 + (i % 5), kid2.sex, fid, campId));
        }
    }

    // Extra children, used only to make camp 2's families a realistic size.
    for (int k = 0; k < extraChildren; k++) {
        const NamedPerson& kid = CHILDREN[childSlot % 12];
        childSlot++;
        std::string name = std::string(kid.name) + " " + surname;
        f->addMember(new Child(name, 3 + (i + k) % 10, kid.sex, fid, campId));
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
// Picks the shelter with the lowest fraction filled that can still take the
// whole family, so the families spread over the rooms. -1 when none can.
static int chooseShelter(const Camp& camp, int people) {
    int best = -1;
    double bestFill = 2.0;
    for (int s = 0; s < camp.shelterCount(); s++) {
        const Camp::Shelter& sh = camp.shelterAt(s);
        if (!sh.hasRoom(people)) continue;
        double fill = static_cast<double>(sh.getOccupancy()) / sh.getCapacity();
        if (fill < bestFill) {
            bestFill = fill;
            best = s;
        }
    }
    return best;
}

static void placeFamily(Camp& camp, Family& f) {
    int people = f.size();
    int shelterIndex = chooseShelter(camp, people);
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
            int extraChildren = 0;
            if (camp == camp2) {
                extraChildren = CAMP2_BASE_EXTRA_CHILDREN;
                if (i - CAMP2_FIRST_INDEX < CAMP2_FAMILIES_WITH_ONE_MORE) {
                    extraChildren++;
                }
            }
            std::unique_ptr<Family> f = buildFamily(i, camp->getId(), extraChildren);
            placeFamily(*camp, *f);
            families.add(std::move(f));
        }

        // 4. The four outside volunteers.
        for (int v = 0; v < 4; v++) {
            const VolunteerRow& row = OUTSIDE_VOLUNTEERS[v];
            volunteers.add(std::unique_ptr<Volunteer>(new Volunteer(
                row.name, row.age, row.sex, row.skill, row.shift, row.organisation)));
        }

        // 5. Stock QUANTITIES. Ask CsvStore for the demand the loader will
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
                // 8. One ledger line per camp per item. loadTxn records the
                // line without touching the stock set above.
                ledger.loadTxn(StockTxn(camp->getId(), item, quantity, true,
                                        allocationDay, "initial district allocation"));
            }
        }

        // 6. A full round at camps 1 and 3 on the day before the demo.
        // Nothing at all is recorded for the demo date.
        for (int i = 0; i < families.size(); i++) {
            const Family* f = families.at(i);
            if (f->getCampId() == 1 || f->getCampId() == 3) {
                log.loadEntry(Allocation(f->getId(), f->getCampId(), lastRound, f->dailyRation()));
            }
        }

        // 7. Health register. Camp 2 sits at 7 diarrhoea cases, below 10.
        health.logCase(2, allocationDay, Symptom::Diarrhoea, 3);
        health.logCase(2, lastRound, Symptom::Diarrhoea, 4);
        health.logCase(1, lastRound, Symptom::Fever, 2);
        health.logCase(3, lastRound, Symptom::SkinInfection, 1);

        // 9. Save through the real store and print the summary.
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
