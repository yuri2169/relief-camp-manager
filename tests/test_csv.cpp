#include "TestRunner.h"
#include "Csv.h"
#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Resident.h"
#include "CsvReader.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <memory>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <sstream>

// ---------------------------------------------------------------- helpers
// The files these tests write go to a scratch directory, never to data/, so
// running the suite cannot clobber the committed demo dataset.
static std::string scratchDir() {
    const char* names[] = {"TMPDIR", "TEMP", "TMP"};
    for (int i = 0; i < 3; i++) {
        const char* value = std::getenv(names[i]);
        if (value == 0 || value[0] == '\0') {
            continue;
        }
        std::string dir = value;
        while (dir.size() > 1 && (dir[dir.size() - 1] == '/' || dir[dir.size() - 1] == '\\')) {
            dir.erase(dir.size() - 1);
        }
        // Use it only when we can really write there.
        std::string probe = dir + "/relief_csv_probe.tmp";
        std::ofstream test(probe.c_str());
        if (test.is_open()) {
            test.close();
            std::remove(probe.c_str());
            return dir;
        }
    }
    return ".";
}

static const char* const kFileNames[] = {"camps.csv", "shelters.csv", "stock.csv",
                                         "families.csv", "members.csv", "volunteers.csv",
                                         "distributions.csv", "stock_txns.csv", "health.csv"};

static void removeDataFiles(const std::string& dir) {
    for (int i = 0; i < 9; i++) {
        std::remove((dir + "/" + kFileNames[i]).c_str());
    }
}

static void writeFile(const std::string& dir, const std::string& name, const std::string& text) {
    std::ofstream out((dir + "/" + name).c_str(), std::ios::binary);
    out << text;
}

static std::string readFile(const std::string& dir, const std::string& name) {
    std::ifstream in((dir + "/" + name).c_str(), std::ios::binary);
    std::string text;
    std::string line;
    while (std::getline(in, line)) {
        text += line + "\n";
    }
    return text;
}

// Everything a CsvStore saves or loads, in one place.
static std::string intStr(int v) {
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

struct World {
    Registry<Camp> camps;
    Registry<Family> families;
    Registry<Volunteer> volunteers;
    DistributionLog log;
    StockLedger ledger;
    HealthMonitor health;
};

static void saveWorld(const CsvStore& store, const World& w) {
    store.saveAll(w.camps, w.families, w.volunteers, w.log, w.ledger, w.health);
}

static void loadWorld(const CsvStore& store, World& w) {
    store.loadAll(w.camps, w.families, w.volunteers, w.log, w.ledger, w.health);
}

// Loads into a throwaway world and reports whether the load failed with a
// FileFormatException naming `file` at `line`.
static bool failsAt(const CsvStore& store, const std::string& file, int line) {
    World scratch;
    bool caught = false;
    try {
        loadWorld(store, scratch);
    } catch (const FileFormatException& e) {
        caught = e.getFile() == file && e.getLine() == line &&
                 std::string(e.what()).find(file) != std::string::npos;
    }
    return caught;
}

// Overwrites one saved file with `text` and checks the load names that file
// and line.
static bool badFileFailsAt(const CsvStore& store, const World& good, const std::string& dir,
                           const std::string& file, const std::string& text, int line) {
    saveWorld(store, good);
    writeFile(dir, file, text);
    return failsAt(store, file, line);
}

static const char* const kCampsHeader = "id,name,district,capacity,occupancy\n";
static const char* const kSheltersHeader = "campId,label,capacity,occupancy\n";
static const char* const kStockHeader = "campId,item,quantity\n";
static const char* const kFamiliesHeader = "id,headName,village,campId\n";
static const char* const kMembersHeader =
    "person_id,familyId,typeTag,name,age,sex,pregnant,disabled,lactating,skill,shift,shelterLabel\n";
static const char* const kVolunteersHeader = "id,name,age,sex,skill,shift,organisation\n";
static const char* const kDistHeader =
    "familyId,campId,date,Rice,Dal,Oil,Salt,ORS,Baby food,Tarpaulin\n";
static const char* const kTxnHeader = "campId,item,quantity,incoming,date,note\n";
static const char* const kHealthHeader = "campId,date,symptom,count\n";

// Builds the state used by the round trip. Returns pointers into the world.
struct Built {
    Camp* school;
    Camp* hall;
    Family* family;
    Family* second;
};

static const char* const kAwkwardHead = "  Bipul \"Bapu\", Das  ";

static Built buildWorld(World& w, const Date& today) {
    Built b;
    b.school = w.camps.add(std::unique_ptr<Camp>(
        new Camp("Bhuragaon HS School", "Morigaon", 200)));
    b.school->addShelter(Camp::Shelter("Room-1", 60));
    b.school->addShelter(Camp::Shelter("Tent-1", 40));
    b.school->stockRef().set(Item::Rice, 500.0);
    b.school->stockRef().set(Item::Dal, 60.5);

    b.hall = w.camps.add(std::unique_ptr<Camp>(
        new Camp("Laharighat Community Hall", "Morigaon", 150)));
    b.hall->stockRef().set(Item::Rice, 120.0);

    // The head's name has a comma, quotes and spaces at both ends: the awkward
    // strings that a naive CSV writer breaks.
    b.family = w.families.add(std::unique_ptr<Family>(new Family(kAwkwardHead, "Bhuragaon")));
    Family* f = b.family;
    f->setCampId(b.school->getId());
    Adult* bipul = new Adult("Bipul Das", 35, 'M', f->getId(), b.school->getId());
    bipul->assignTo(b.school->getId(), "Room-1");       // placed; the others are not
    f->addMember(bipul);
    Adult* mother = new Adult("Rina Das", 29, 'F', f->getId(), b.school->getId());
    mother->setFlags(true, false, false);               // pregnant - must survive the trip
    f->addMember(mother);
    Child* rahul = new Child("Rahul \"Raju\" Das, Jr", 9, 'M', f->getId(), b.school->getId());
    rahul->setFlags(false, true, false);                // disabled
    f->addMember(rahul);
    Infant* mitu = new Infant("Mitu Das", 1, 'F', f->getId(), b.school->getId());
    f->addMember(mitu);
    f->addMember(new Elderly("Minoti Devi", 72, 'F', f->getId(), b.school->getId()));
    f->addMember(new ResidentVolunteer("Gautam Das", 31, 'M', f->getId(),
                                       b.school->getId(), "Record keeper", "Evening"));
    b.school->admit(*f);
    // Place 4 of the 6 people in Room-1: shelter occupancy is placement detail.
    b.school->shelterAt(0).occupy(4);

    // A second family waiting in the hall, with a lactating mother.
    b.second = w.families.add(std::unique_ptr<Family>(new Family("Anita Boro", "Laharighat")));
    b.second->setCampId(b.hall->getId());
    Adult* anita = new Adult("Anita Boro", 27, 'F', b.second->getId(), b.hall->getId());
    anita->setFlags(false, false, true);
    b.second->addMember(anita);
    b.second->addMember(new Infant("Baby Boro", 0, 'M', b.second->getId(), b.hall->getId()));
    b.hall->admit(*b.second);

    // Daily demand is derived: the sum of its families' rations.
    b.school->stockRef().setDailyDemand(f->dailyRation());
    b.hall->stockRef().setDailyDemand(b.second->dailyRation());

    w.volunteers.add(std::unique_ptr<Volunteer>(
        new Volunteer("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross")));
    w.volunteers.add(std::unique_ptr<Volunteer>(
        new Volunteer("Nilufa, \"Nilu\" Begum", 38, 'F', "Cook", "Night", "Sewa, Assam")));

    w.log.record(Allocation(f->getId(), b.school->getId(), today, f->dailyRation()));
    w.log.record(Allocation(b.second->getId(), b.hall->getId(), today.plusDays(1),
                            b.second->dailyRation()));
    w.ledger.stockIn(*b.school, Item::Rice, 100.0, today, "district supply, \"urgent\"");
    w.ledger.stockOut(*b.school, Item::Dal, 10.25, today.plusDays(1), "");
    w.health.logCase(b.school->getId(), today, Symptom::Diarrhoea, 11);
    w.health.logCase(b.hall->getId(), today.plusDays(1), Symptom::SkinInfection, 3);
    return b;
}

void run_csv_tests() {
    testing::beginSuite("Csv helpers and CsvStore round trip");

    // --- Csv field handling ---
    std::vector<std::string> fields = Csv::split("1,Bhuragaon HS School,Morigaon,200,45");
    ASSERT_EQ(static_cast<int>(fields.size()), 5);
    ASSERT_STR_EQ(fields[1], "Bhuragaon HS School");
    ASSERT_STR_EQ(fields[4], "45");

    // Empty fields are preserved, so a missing shelter label does not shift
    // every later column.
    std::vector<std::string> sparse = Csv::split("1,,Morigaon,,45");
    ASSERT_EQ(static_cast<int>(sparse.size()), 5);
    ASSERT_STR_EQ(sparse[1], "");
    ASSERT_STR_EQ(sparse[3], "");
    ASSERT_EQ(static_cast<int>(Csv::split("a,b,").size()), 3);      // trailing comma
    ASSERT_EQ(static_cast<int>(Csv::split("").size()), 1);          // one empty field

    // A field containing a comma is quoted on the way out and recovered on the
    // way in. Assamese place names do contain commas in official lists.
    ASSERT_STR_EQ(Csv::escape("Mayong, Morigaon"), "\"Mayong, Morigaon\"");
    ASSERT_STR_EQ(Csv::escape("plain"), "plain");
    ASSERT_STR_EQ(Csv::escape("say \"hi\""), "\"say \"\"hi\"\"\"");
    std::vector<std::string> quoted = Csv::split("1,\"Mayong, Morigaon\",200");
    ASSERT_EQ(static_cast<int>(quoted.size()), 3);
    ASSERT_STR_EQ(quoted[1], "Mayong, Morigaon");

    std::vector<std::string> out;
    out.push_back("1");
    out.push_back("Mayong, Morigaon");
    out.push_back("200");
    ASSERT_STR_EQ(Csv::join(out), "1,\"Mayong, Morigaon\",200");

    // Whatever escape writes, split must give back exactly: commas, quotes,
    // and spaces at both ends.
    std::vector<std::string> awkward;
    awkward.push_back(kAwkwardHead);
    awkward.push_back("\"");
    awkward.push_back("");
    awkward.push_back(" , ");
    awkward.push_back("a\"\"b");
    std::vector<std::string> back = Csv::split(Csv::join(awkward));
    ASSERT_EQ(static_cast<int>(back.size()), 5);
    for (int i = 0; i < 5; i++) {
        ASSERT_STR_EQ(back[i], awkward[i]);
    }

    // Numeric parsing names the file and line when the data is wrong, because
    // these files get hand-edited in the field.
    ASSERT_EQ(Csv::toInt("42", "camps.csv", 3), 42);
    ASSERT_EQ(Csv::toInt("-7", "camps.csv", 3), -7);
    ASSERT_NEAR(Csv::toDouble("12.5", "stock.csv", 7), 12.5, 1e-9);
    ASSERT_THROWS(Csv::toInt("forty-two", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toInt("12abc", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toInt("4.5", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toInt("", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toInt(" 5", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toInt("99999999999", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toDouble("", "stock.csv", 7), FileFormatException);
    ASSERT_THROWS(Csv::toDouble("1.5kg", "stock.csv", 7), FileFormatException);
    ASSERT_THROWS(Csv::toDouble("nan", "stock.csv", 7), FileFormatException);
    ASSERT_THROWS(Csv::toDouble("inf", "stock.csv", 7), FileFormatException);
    bool caught = false;
    try {
        Csv::toInt("abc", "camps.csv", 9);
    } catch (const FileFormatException& e) {
        caught = true;
        ASSERT_STR_EQ(e.getFile(), "camps.csv");
        ASSERT_EQ(e.getLine(), 9);
        ASSERT_TRUE(std::string(e.what()).find("abc") != std::string::npos);
    }
    ASSERT_TRUE(caught);
    ASSERT_THROWS(Csv::require(3, 5, "families.csv", 12), FileFormatException);
    Csv::require(5, 5, "families.csv", 12);                        // equal: no throw

    // --- makePerson turns a tag back into the right concrete class ---
    {
        const char* tags[] = {"INFANT", "CHILD", "ADULT", "ELDERLY", "RESIDENT_VOLUNTEER"};
        for (int i = 0; i < 5; i++) {
            std::unique_ptr<Person> p(CsvStore::makePerson(tags[i], "Test", 20, 'F', 1, 2,
                                                           "Cook", "Day"));
            ASSERT_TRUE(p.get() != 0);
            ASSERT_STR_EQ(p->typeTag(), tags[i]);
            ASSERT_STR_EQ(p->getName(), "Test");
        }
        ASSERT_THROWS(CsvStore::makePerson("VOLUNTEER", "X", 1, 'M', 1, 1, "Cook", "Day"),
                      std::invalid_argument);
        ASSERT_THROWS(CsvStore::makePerson("ROBOT", "X", 1, 'M', 1, 1, "", ""),
                      std::invalid_argument);
        ASSERT_THROWS(CsvStore::makePerson("", "X", 1, 'M', 1, 1, "", ""),
                      std::invalid_argument);
        ASSERT_THROWS(CsvStore::makePerson("adult", "X", 1, 'M', 1, 1, "", ""),
                      std::invalid_argument);
    }

    // --- full round trip ---
    const std::string dir = scratchDir();
    removeDataFiles(dir);
    CsvStore store(dir);
    ASSERT_STR_EQ(store.getDir(), dir);
    ASSERT_FALSE(store.dataExists());

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    Date today(2026, 7, 14);
    World w;
    Built b = buildWorld(w, today);
    Camp* school = b.school;
    Family* f = b.family;

    int familyCount = w.families.size();
    int memberCount = f->size();
    double savedRice = school->getStock().get(Item::Rice);
    int savedOccupancy = school->getOccupancy();
    int savedFamilyId = f->getId();
    int secondFamilyId = b.second->getId();
    double savedFamilyRice = f->dailyRation().get(Item::Rice);
    int savedVulnerability = f->vulnerabilityScore();
    int savedHallId = b.hall->getId();
    int savedSchoolId = school->getId();

    saveWorld(store, w);
    ASSERT_TRUE(store.dataExists());

    // The file is plain text with a header row: what a spreadsheet would show.
    std::string campsText = readFile(dir, "camps.csv");
    ASSERT_TRUE(campsText.find("id,name,district,capacity,occupancy\n") == 0);
    ASSERT_TRUE(campsText.find("1,Bhuragaon HS School,Morigaon,200,6\n") != std::string::npos);
    ASSERT_TRUE(readFile(dir, "distributions.csv").find(
        "familyId,campId,date,Rice,Dal,Oil,Salt,ORS,Baby food,Tarpaulin\n") == 0);
    ASSERT_TRUE(readFile(dir, "health.csv").find("2,2026-07-15,Skin infection,3\n")
                != std::string::npos);

    // Load into a completely fresh set of registries.
    World w2;
    loadWorld(store, w2);

    ASSERT_EQ(w2.camps.size(), 2);
    ASSERT_EQ(w2.families.size(), familyCount);
    ASSERT_EQ(w2.volunteers.size(), 2);

    // IDs survive, which is what makes the cross-references work.
    Camp* school2 = w2.camps.findById(savedSchoolId);
    ASSERT_TRUE(school2 != 0);
    ASSERT_STR_EQ(school2->getName(), "Bhuragaon HS School");
    ASSERT_STR_EQ(school2->getDistrict(), "Morigaon");
    ASSERT_EQ(school2->getCapacity(), 200);
    ASSERT_EQ(school2->getOccupancy(), savedOccupancy);
    ASSERT_EQ(school2->shelterCount(), 2);
    ASSERT_STR_EQ(school2->shelterAt(0).getLabel(), "Room-1");
    ASSERT_EQ(school2->shelterAt(0).getCapacity(), 60);
    ASSERT_EQ(school2->shelterAt(0).getOccupancy(), 4);   // placement detail restored as saved
    ASSERT_STR_EQ(school2->shelterAt(1).getLabel(), "Tent-1");
    ASSERT_EQ(school2->shelterAt(1).getCapacity(), 40);
    ASSERT_EQ(school2->shelterAt(1).getOccupancy(), 0);
    ASSERT_NEAR(school2->getStock().get(Item::Rice), savedRice, 1e-9);
    ASSERT_NEAR(school2->getStock().get(Item::Dal), 50.25, 1e-9);
    ASSERT_NEAR(school2->getStock().get(Item::Oil), 0.0, 1e-9);
    Camp* hall2 = w2.camps.findById(savedHallId);
    ASSERT_TRUE(hall2 != 0);
    ASSERT_STR_EQ(hall2->getName(), "Laharighat Community Hall");
    ASSERT_EQ(hall2->getOccupancy(), 2);
    ASSERT_EQ(hall2->shelterCount(), 0);
    ASSERT_NEAR(hall2->getStock().get(Item::Rice), 120.0, 1e-9);

    // Every item of every camp's stock, and the derived daily demand.
    for (int c = 0; c < 2; c++) {
        Camp* original = (c == 0) ? school : b.hall;
        Camp* loaded = w2.camps.findById(original->getId());
        for (int i = 0; i < ITEM_COUNT; i++) {
            Item it = itemAt(i);
            ASSERT_NEAR(loaded->getStock().get(it), original->getStock().get(it), 1e-9);
            ASSERT_NEAR(loaded->getStock().getDailyDemand().get(it),
                        original->getStock().getDailyDemand().get(it), 1e-9);
        }
    }

    Family* f2 = w2.families.findById(savedFamilyId);
    ASSERT_TRUE(f2 != 0);
    ASSERT_STR_EQ(f2->getHeadName(), kAwkwardHead);       // comma, quotes, edge spaces
    ASSERT_STR_EQ(f2->getVillage(), "Bhuragaon");
    ASSERT_EQ(f2->getCampId(), savedSchoolId);
    ASSERT_EQ(f2->size(), memberCount);

    // The dynamic type of every member survives the round trip. This is the
    // assertion that catches a CSV layer that quietly turns everyone into an
    // Adult: the ration and the score would both be wrong.
    ASSERT_EQ(f2->countOfTag("ADULT"), 2);
    ASSERT_EQ(f2->countOfTag("CHILD"), 1);
    ASSERT_EQ(f2->countOfTag("INFANT"), 1);
    ASSERT_EQ(f2->countOfTag("ELDERLY"), 1);
    ASSERT_EQ(f2->countOfTag("RESIDENT_VOLUNTEER"), 1);
    ASSERT_NEAR(f2->dailyRation().get(Item::Rice), savedFamilyRice, 1e-9);
    ASSERT_EQ(f2->vulnerabilityScore(), savedVulnerability);

    // Member by member, in order: type, name, age, sex and all three flags.
    for (int i = 0; i < f->size(); i++) {
        const Person& a = f->memberAt(i);
        const Person& c = f2->memberAt(i);
        ASSERT_STR_EQ(c.typeTag(), a.typeTag());
        ASSERT_STR_EQ(c.getName(), a.getName());
        ASSERT_EQ(c.getAge(), a.getAge());
        ASSERT_EQ(static_cast<int>(c.getSex()), static_cast<int>(a.getSex()));
        ASSERT_EQ(c.isPregnant() ? 1 : 0, a.isPregnant() ? 1 : 0);
        ASSERT_EQ(c.isDisabled() ? 1 : 0, a.isDisabled() ? 1 : 0);
        ASSERT_EQ(c.isLactating() ? 1 : 0, a.isLactating() ? 1 : 0);
        ASSERT_EQ(c.vulnerabilityScore(), a.vulnerabilityScore());
        ASSERT_TRUE(c.rationNeed() == a.rationNeed());
        const Resident* ra = dynamic_cast<const Resident*>(&a);
        const Resident* rc = dynamic_cast<const Resident*>(&c);
        ASSERT_TRUE(ra != 0 && rc != 0);
        if (ra != 0 && rc != 0) {
            ASSERT_STR_EQ(rc->getShelterLabel(), ra->getShelterLabel());
            ASSERT_EQ(rc->getCampId(), ra->getCampId());
            ASSERT_EQ(rc->getFamilyId(), ra->getFamilyId());
        }
    }
    ASSERT_STR_EQ(dynamic_cast<const Resident&>(f2->memberAt(0)).getShelterLabel(), "Room-1");
    ASSERT_STR_EQ(dynamic_cast<const Resident&>(f2->memberAt(1)).getShelterLabel(), "");
    ASSERT_STR_EQ(f2->memberAt(2).getName(), "Rahul \"Raju\" Das, Jr");
    ASSERT_TRUE(f2->memberAt(2).isDisabled());

    // The pregnancy flag survived, so the bonus ration is still right.
    bool foundPregnant = false;
    for (int i = 0; i < f2->size(); i++) {
        if (f2->memberAt(i).isPregnant()) { foundPregnant = true; }
    }
    ASSERT_TRUE(foundPregnant);

    // The resident volunteer kept both halves of its identity.
    bool foundRv = false;
    for (int i = 0; i < f2->size(); i++) {
        if (f2->memberAt(i).typeTag() == "RESIDENT_VOLUNTEER") {
            foundRv = true;
            ASSERT_NEAR(f2->memberAt(i).rationNeed().get(Item::Rice), 0.400, 1e-9);
            const ResidentVolunteer* rv = dynamic_cast<const ResidentVolunteer*>(&f2->memberAt(i));
            ASSERT_TRUE(rv != 0);
            if (rv != 0) {
                ASSERT_STR_EQ(rv->getSkill(), "Record keeper");
                ASSERT_STR_EQ(rv->getShift(), "Evening");
            }
        }
    }
    ASSERT_TRUE(foundRv);

    Family* second2 = w2.families.findById(secondFamilyId);
    ASSERT_TRUE(second2 != 0);
    ASSERT_STR_EQ(second2->getHeadName(), "Anita Boro");
    ASSERT_EQ(second2->getCampId(), savedHallId);
    ASSERT_EQ(second2->size(), 2);
    ASSERT_TRUE(second2->memberAt(0).isLactating());

    // Outside volunteers: a separate registry, never mixed with family members.
    Volunteer* doc = w2.volunteers.at(0);
    ASSERT_TRUE(doc != 0);
    ASSERT_EQ(doc->getId(), w.volunteers.at(0)->getId());
    ASSERT_STR_EQ(doc->getName(), "Dr Priyam Deka");
    ASSERT_EQ(doc->getAge(), 45);
    ASSERT_EQ(static_cast<int>(doc->getSex()), static_cast<int>('M'));
    ASSERT_STR_EQ(doc->getSkill(), "Medic");
    ASSERT_STR_EQ(doc->getShift(), "Morning");
    ASSERT_STR_EQ(doc->getOrganisation(), "Red Cross");
    ASSERT_STR_EQ(w2.volunteers.at(1)->getName(), "Nilufa, \"Nilu\" Begum");
    ASSERT_STR_EQ(w2.volunteers.at(1)->getOrganisation(), "Sewa, Assam");
    ASSERT_EQ(w2.volunteers.at(1)->getId(), w.volunteers.at(1)->getId());

    // Logs survive.
    ASSERT_EQ(w2.log.size(), 2);
    ASSERT_TRUE(w2.log.alreadyClaimed(savedFamilyId, today));
    ASSERT_FALSE(w2.log.alreadyClaimed(savedFamilyId, today.plusDays(1)));
    ASSERT_TRUE(w2.log.alreadyClaimed(secondFamilyId, today.plusDays(1)));
    ASSERT_NEAR(w2.log.totalForCamp(savedSchoolId, today).get(Item::Rice),
                savedFamilyRice, 1e-9);
    for (int i = 0; i < w.log.size(); i++) {
        const Allocation& a = w.log.at(i);
        const Allocation& c = w2.log.at(i);
        ASSERT_EQ(c.getFamilyId(), a.getFamilyId());
        ASSERT_EQ(c.getCampId(), a.getCampId());
        ASSERT_TRUE(c.getDate() == a.getDate());
        for (int k = 0; k < ITEM_COUNT; k++) {
            ASSERT_NEAR(c.getAmount().get(itemAt(k)), a.getAmount().get(itemAt(k)), 1e-9);
        }
    }

    ASSERT_EQ(w2.ledger.size(), 2);
    for (int i = 0; i < w.ledger.size(); i++) {
        const StockTxn& a = w.ledger.at(i);
        const StockTxn& c = w2.ledger.at(i);
        ASSERT_EQ(c.getCampId(), a.getCampId());
        ASSERT_TRUE(c.getItem() == a.getItem());
        ASSERT_NEAR(c.getQuantity(), a.getQuantity(), 1e-9);
        ASSERT_EQ(c.isIncoming() ? 1 : 0, a.isIncoming() ? 1 : 0);
        ASSERT_TRUE(c.getDate() == a.getDate());
        ASSERT_STR_EQ(c.getNote(), a.getNote());
    }
    ASSERT_STR_EQ(w2.ledger.at(0).getNote(), "district supply, \"urgent\"");
    ASSERT_STR_EQ(w2.ledger.at(1).getNote(), "");
    ASSERT_NEAR(w2.ledger.netFor(savedSchoolId, Item::Dal), -10.25, 1e-9);

    ASSERT_EQ(w2.health.size(), 2);
    for (int i = 0; i < w.health.size(); i++) {
        const HealthCase& a = w.health.at(i);
        const HealthCase& c = w2.health.at(i);
        ASSERT_EQ(c.getCampId(), a.getCampId());
        ASSERT_TRUE(c.getDate() == a.getDate());
        ASSERT_TRUE(c.getSymptom() == a.getSymptom());
        ASSERT_EQ(c.getCount(), a.getCount());
    }
    ASSERT_TRUE(w2.health.at(1).getSymptom() == Symptom::SkinInfection);   // "Skin infection"
    ASSERT_TRUE(w2.health.outbreakAlert(savedSchoolId, today, Symptom::Diarrhoea));

    // Person ids of family members survive the trip, member by member.
    for (int i = 0; i < f->size(); i++) {
        ASSERT_EQ(f2->memberAt(i).getId(), f->memberAt(i).getId());
    }
    for (int i = 0; i < b.second->size(); i++) {
        ASSERT_EQ(second2->memberAt(i).getId(), b.second->memberAt(i).getId());
    }

    // A save leaves no temporary files behind.
    for (int i = 0; i < 9; i++) {
        std::ifstream leftover((dir + "/" + kFileNames[i] + ".tmp").c_str());
        ASSERT_FALSE(leftover.is_open());
    }

    // Saving what was loaded gives byte-for-byte the same files.
    std::string before[9];
    for (int i = 0; i < 9; i++) { before[i] = readFile(dir, kFileNames[i]); }
    saveWorld(store, w2);
    for (int i = 0; i < 9; i++) {
        ASSERT_TRUE(readFile(dir, kFileNames[i]) == before[i]);
    }

    // A new object created after loading must not collide with a loaded ID,
    // for every kind of ID the system hands out.
    Family* fresh = w2.families.add(std::unique_ptr<Family>(new Family("New Head", "New Village")));
    ASSERT_TRUE(w2.families.findById(fresh->getId()) == fresh);
    ASSERT_TRUE(fresh->getId() != savedFamilyId);
    ASSERT_TRUE(fresh->getId() > secondFamilyId);

    Camp* freshCamp = w2.camps.add(std::unique_ptr<Camp>(new Camp("New Camp", "Nagaon", 50)));
    ASSERT_TRUE(freshCamp->getId() > savedHallId);
    ASSERT_TRUE(freshCamp->getId() > savedSchoolId);
    ASSERT_EQ(w2.camps.size(), 3);

    int maxPersonId = 0;
    for (int i = 0; i < w2.volunteers.size(); i++) {
        if (w2.volunteers.at(i)->getId() > maxPersonId) { maxPersonId = w2.volunteers.at(i)->getId(); }
    }
    for (int i = 0; i < w2.families.size(); i++) {
        for (int m = 0; m < w2.families.at(i)->size(); m++) {
            if (w2.families.at(i)->memberAt(m).getId() > maxPersonId) {
                maxPersonId = w2.families.at(i)->memberAt(m).getId();
            }
        }
    }
    Adult newcomer("Newcomer", 30, 'M', fresh->getId(), freshCamp->getId());
    ASSERT_TRUE(newcomer.getId() > maxPersonId);
    // Nobody in the loaded world shares a Person id with anybody else.
    {
        std::vector<int> seen;
        for (int i = 0; i < w2.volunteers.size(); i++) { seen.push_back(w2.volunteers.at(i)->getId()); }
        for (int i = 0; i < w2.families.size(); i++) {
            for (int m = 0; m < w2.families.at(i)->size(); m++) {
                seen.push_back(w2.families.at(i)->memberAt(m).getId());
            }
        }
        bool duplicate = false;
        for (std::size_t x = 0; x < seen.size(); x++) {
            for (std::size_t y = x + 1; y < seen.size(); y++) {
                if (seen[x] == seen[y]) { duplicate = true; }
            }
        }
        ASSERT_FALSE(duplicate);
    }

    // Loading twice into the same objects replaces, it does not duplicate.
    saveWorld(store, w);
    loadWorld(store, w2);
    ASSERT_EQ(w2.camps.size(), 2);
    ASSERT_EQ(w2.families.size(), familyCount);
    ASSERT_EQ(w2.log.size(), 2);

    // --- IDs of a state that does NOT start at 1 (a camp deleted earlier) ---
    {
        World gap;
        Camp::resetIdCounter(40);
        Family::resetIdCounter(70);
        Camp* c = gap.camps.add(std::unique_ptr<Camp>(new Camp("Gap Camp", "Jorhat", 10)));
        Family* g = gap.families.add(std::unique_ptr<Family>(new Family("Gap Head", "V")));
        g->addMember(new Adult("Gap Adult", 30, 'M', g->getId(), c->getId()));
        gap.volunteers.add(std::unique_ptr<Volunteer>(
            new Volunteer("Gap Vol", 30, 'M', "Cook", "Day", "NGO")));
        int campId = c->getId();
        int familyId = g->getId();
        int volId = gap.volunteers.at(0)->getId();
        saveWorld(store, gap);

        // The new session starts its counters from scratch.
        Person::resetIdCounter(1);
        Family::resetIdCounter(1);
        Camp::resetIdCounter(1);
        World loaded;
        loadWorld(store, loaded);
        ASSERT_TRUE(loaded.camps.findById(campId) != 0);
        ASSERT_TRUE(loaded.families.findById(familyId) != 0);
        ASSERT_EQ(loaded.volunteers.at(0)->getId(), volId);
        ASSERT_EQ(Camp::generateId(), campId + 1);
        ASSERT_EQ(Family::generateId(), familyId + 1);
        Adult next("Next", 20, 'F', 1, 1);
        ASSERT_TRUE(next.getId() > volId);
        ASSERT_TRUE(next.getId() > loaded.families.at(0)->memberAt(0).getId());
    }

    // --- malformed data is reported with the file and the line number ---
    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
    World good;
    buildWorld(good, today);
    const std::string c1 = "1,Good Camp,Morigaon,200,45\n";

    {
        // The brief's own case: only 3 fields on line 3 (the header is line 1).
        writeFile(dir, "camps.csv", std::string(kCampsHeader) + c1 + "2,Broken Camp,Morigaon\n");
        World bad;
        ASSERT_THROWS(loadWorld(store, bad), FileFormatException);
        ASSERT_TRUE(failsAt(store, "camps.csv", 3));
    }

    // camps.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + c1 + "2,Camp,Morigaon,200,1,extra\n", 3));      // too many
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "x,Camp,Morigaon,200,1\n", 2));                 // bad id
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "0,Camp,Morigaon,200,1\n", 2));                 // id not positive
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "1,Camp,Morigaon,two hundred,1\n", 2));         // bad number
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "1,Camp,Morigaon,-5,0\n", 2));                  // negative capacity
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + c1 + "2,Camp,Morigaon,100,101\n", 3));          // over capacity
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "1,Camp,Morigaon,100,-1\n", 2));                // negative occupancy
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + c1 + "1,Same Id,Morigaon,100,1\n", 3));         // duplicate id
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        "id,name,district,capacity\n" + c1, 1));                                     // wrong header
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "camps.csv",
        std::string(kCampsHeader) + "1,\"Unclosed,Morigaon,200,1\n", 2));           // unterminated quote

    // shelters.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "shelters.csv",
        std::string(kSheltersHeader) + "99,Room-1,60,0\n", 2));                      // unknown camp
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "shelters.csv",
        std::string(kSheltersHeader) + "1,Room-1,60,61\n", 2));                      // over capacity
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "shelters.csv",
        std::string(kSheltersHeader) + "1,Room-1,60,-1\n", 2));                      // negative
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "shelters.csv",
        std::string(kSheltersHeader) + "1,Room-1,-60,0\n", 2));                      // negative capacity
    // The model never keeps shelters and head-count in lockstep (release() does
    // not vacate a shelter), so shelters holding more than the camp must LOAD.
    {
        saveWorld(store, good);
        writeFile(dir, "shelters.csv", std::string(kSheltersHeader) + "1,Room-1,60,40\n" +
                  "1,Tent-1,40,30\n");
        World lax;
        loadWorld(store, lax);
        ASSERT_EQ(lax.camps.findById(1)->shelterAt(0).getOccupancy(), 40);
        ASSERT_EQ(lax.camps.findById(1)->shelterAt(1).getOccupancy(), 30);
        ASSERT_EQ(lax.camps.findById(1)->getOccupancy(), 6);
    }
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "shelters.csv",
        std::string(kSheltersHeader) + "1,Room-1,60\n", 2));                         // too few

    // stock.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "1,Rice,10\n" + "1,Gold,5\n", 3));               // unknown item
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "1,Rice,10\n" + "2,Rice,10\n" + "1,Rice,11\n", 4));   // duplicate row
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "1,Rice,-5\n", 2));                              // negative quantity
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "1,Rice,lots\n", 2));                            // not a number
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "77,Rice,5\n", 2));                              // unknown camp
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock.csv",
        std::string(kStockHeader) + "1,rice,5\n", 2));                               // case matters

    // families.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "families.csv",
        std::string(kFamiliesHeader) + "1,Head,Village,55\n", 2));                   // unknown camp
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "families.csv",
        std::string(kFamiliesHeader) + "1,Head,Village,0\n" + "1,Other,Village,0\n", 3));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "families.csv",
        std::string(kFamiliesHeader) + "1,Head,Village\n", 2));

    // members.csv
    const std::string m1 = "901,1,ADULT,Bipul Das,35,M,0,0,0,,,\n";
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + m1 + "902,1,ROBOT,Rob,3,M,0,0,0,,,\n", 3));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + m1 + "901,1,ADULT,Same Id,3,M,0,0,0,,,\n", 3));   // duplicate person id         // unknown tag
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,1,VOLUNTEER,Vol,30,M,0,0,0,Cook,Day,\n", 2));  // not a family member
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,1,ADULT,Bipul Das,35,M,yes,0,0,,,\n", 2));     // bad flag
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,1,ADULT,Bipul Das,35,X,0,0,0,,,\n", 2));       // bad sex
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,1,ADULT,Bipul Das,old,M,0,0,0,,,\n", 2));      // bad age
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,1,ADULT,Bipul Das,-3,M,0,0,0,,,\n", 2));       // negative age
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + "903,99,ADULT,Bipul Das,35,M,0,0,0,,,\n", 2));      // unknown family
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "members.csv",
        std::string(kMembersHeader) + m1 + "902,1,ADULT,Two,35,M,0,0,0,,,\n" + "903,1,ADULT,Short row\n", 4));          // too few fields

    // A member may not reuse an outside volunteer's person id either.
    {
        saveWorld(store, good);
        int volunteerId = good.volunteers.at(0)->getId();
        writeFile(dir, "members.csv", std::string(kMembersHeader) + intStr(volunteerId) +
                  ",1,ADULT,Clash,35,M,0,0,0,,,\n");
        ASSERT_TRUE(failsAt(store, "members.csv", 2));
    }

    // volunteers.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "volunteers.csv",
        std::string(kVolunteersHeader) + "9,Vol,thirty,M,Cook,Day,NGO\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "volunteers.csv",
        std::string(kVolunteersHeader) + "9,Vol,30,M,Cook,Day,NGO\n" + "9,Vol2,30,M,Cook,Day,NGO\n", 3));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "volunteers.csv",
        std::string(kVolunteersHeader) + "9,Vol,30,Q,Cook,Day,NGO\n", 2));

    // distributions.csv: an invalid date must never reach the log.
    const std::string zeros = ",0,0,0,0,0,0,0\n";
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-02-30" + zeros, 2));                    // no 30 Feb
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-13-01" + zeros, 2));                    // no month 13
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-00-10" + zeros, 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,14/07/2026" + zeros, 2));                    // wrong format
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-07-14" + zeros + "1,1,2026-07-14,1,2,3\n", 3)); // too few
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-07-14" + zeros + "1,2,2026-07-15" + zeros +
        "1,2,2026-07-14" + zeros, 4));                                               // same family+day, other camp
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-07-14,1,1,1,1,1,1,x\n", 2));            // bad quantity
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,1,2026-07-14,1,1,1,1,1,1,-1\n", 2));           // negative quantity

    // Ids in the logs must be positive: 0 usually means a shifted column.
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "0,1,2026-07-14" + zeros, 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "distributions.csv",
        std::string(kDistHeader) + "1,-1,2026-07-14" + zeros, 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "0,Rice,5,1,2026-07-14,note\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "0,2026-07-14,Fever,3\n", 2));

    // stock_txns.csv
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Rice,5,1,2026-02-29,note\n", 2));               // 2026 is not a leap year
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Gold,5,1,2026-07-14,note\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Rice,5,2,2026-07-14,note\n", 2));               // incoming must be 0 or 1
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Rice,0,1,2026-07-14,note\n", 2));               // quantity must be > 0
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Rice,-5,0,2026-07-14,note\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "stock_txns.csv",
        std::string(kTxnHeader) + "1,Rice,5,1,2026-07-14\n", 2));

    // health.csv: the 3-day window depends on every date being real.
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-04-31,Fever,3\n", 2));                  // April has 30 days
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,yesterday,Fever,3\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-07-14,Plague,3\n", 2));                 // unknown symptom
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-07-14,Fever,0\n", 2));                  // count must be > 0
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-07-14,Fever,-4\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-07-14,Fever,many\n", 2));
    ASSERT_TRUE(badFileFailsAt(store, good, dir, "health.csv",
        std::string(kHealthHeader) + "1,2026-07-14,skin infection,2\n", 2));         // exact name only

    // --- tolerant where it should be ---
    {
        // Windows line endings, blank lines and a spreadsheet's byte-order mark.
        saveWorld(store, good);
        writeFile(dir, "camps.csv", std::string("\xEF\xBB\xBF") + "id,name,district,capacity,occupancy\r\n"
                  "5,Windows Camp,Dhubri,80,3\r\n\r\n6,\"Quoted, Camp\",Dhubri,40,0\r\n");
        writeFile(dir, "shelters.csv", "");                       // empty file: nothing to load
        writeFile(dir, "stock.csv", std::string(kStockHeader) + "5,Rice,12.5\r\n");
        writeFile(dir, "families.csv", kFamiliesHeader);          // header only
        writeFile(dir, "members.csv", kMembersHeader);
        World win;
        Camp::resetIdCounter(1);                                   // a fresh session
        loadWorld(store, win);
        ASSERT_EQ(win.camps.size(), 2);
        ASSERT_STR_EQ(win.camps.findById(5)->getName(), "Windows Camp");
        ASSERT_EQ(win.camps.findById(5)->getOccupancy(), 3);
        ASSERT_STR_EQ(win.camps.findById(6)->getName(), "Quoted, Camp");
        ASSERT_NEAR(win.camps.findById(5)->getStock().get(Item::Rice), 12.5, 1e-9);
        ASSERT_EQ(win.families.size(), 0);
        ASSERT_EQ(Camp::generateId(), 7);                          // counter follows the loaded ids
        // ... and never moves backwards past ids already handed out.
        Camp::resetIdCounter(500);
        World again;
        loadWorld(store, again);
        ASSERT_TRUE(Camp::generateId() >= 500);
    }
    {
        // A quoted field may span lines, so a name with a newline is not lost.
        removeDataFiles(dir);                                      // camps.csv alone
        writeFile(dir, "camps.csv", std::string(kCampsHeader) + "1,\"Two\nLines\",Morigaon,200,0\n"
                  "2,Next,Morigaon,10,0\n3,Bad,Morigaon\n");
        ASSERT_TRUE(failsAt(store, "camps.csv", 5));              // lines 2-3 are one record, 4 is next
        writeFile(dir, "camps.csv", std::string(kCampsHeader) + "1,\"Two\nLines\",Morigaon,200,0\n");
        World nl;
        loadWorld(store, nl);
        ASSERT_STR_EQ(nl.camps.at(0)->getName(), "Two\nLines");
    }

    // A load that fails halfway leaves every container empty, not half a camp.
    {
        saveWorld(store, good);
        writeFile(dir, "health.csv", std::string(kHealthHeader) + "1,2026-07-14,Fever,3\n" +
                  "1,2026-07-14,Plague,3\n");
        World half;
        ASSERT_THROWS(loadWorld(store, half), FileFormatException);
        ASSERT_EQ(half.camps.size(), 0);
        ASSERT_EQ(half.families.size(), 0);
        ASSERT_EQ(half.volunteers.size(), 0);
        ASSERT_EQ(half.log.size(), 0);
        ASSERT_EQ(half.ledger.size(), 0);
        ASSERT_EQ(half.health.size(), 0);
        // Previously loaded content is cleared too: state is well-defined.
        World again;
        saveWorld(store, good);
        loadWorld(store, again);
        ASSERT_EQ(again.camps.size(), 2);
        writeFile(dir, "camps.csv", std::string(kCampsHeader) + "1,Bad,X,10,99\n");
        ASSERT_THROWS(loadWorld(store, again), FileFormatException);
        ASSERT_EQ(again.camps.size(), 0);
        ASSERT_EQ(again.families.size(), 0);
        ASSERT_EQ(again.log.size(), 0);
    }

    // The app's own state: admit, place 4 in a shelter, release the family.
    // Camp holds 0 while the shelter still says 4. It must save and load.
    {
        Person::resetIdCounter(1);
        Family::resetIdCounter(1);
        Camp::resetIdCounter(1);
        World used;
        Built ub = buildWorld(used, today);
        ub.school->release(*ub.family);
        ASSERT_EQ(ub.school->getOccupancy(), 0);
        ASSERT_EQ(ub.school->shelterAt(0).getOccupancy(), 4);
        saveWorld(store, used);
        World back;
        loadWorld(store, back);
        ASSERT_EQ(back.camps.findById(ub.school->getId())->getOccupancy(), 0);
        ASSERT_EQ(back.camps.findById(ub.school->getId())->shelterAt(0).getOccupancy(), 4);
    }

    // A save over existing files leaves no .bak or .tmp behind.
    for (int i = 0; i < 9; i++) {
        std::ifstream bak((dir + "/" + kFileNames[i] + ".bak").c_str());
        std::ifstream tmp((dir + "/" + kFileNames[i] + ".tmp").c_str());
        ASSERT_FALSE(bak.is_open());
        ASSERT_FALSE(tmp.is_open());
    }

    // --- CsvReader framing, on its own ---
    {
        const std::string name = "reader_probe.csv";
        const std::string full = dir + "/" + name;
        std::vector<std::string> row;

        // CRLF, BOM, blank lines, and a missing final newline.
        writeFile(dir, name, std::string("\xEF\xBB\xBF") + "a,b\r\n1,2\r\n\r\n3,4");
        {
            CsvReader r(full, name, 2);
            ASSERT_TRUE(r.isOpen());
            ASSERT_TRUE(r.next(row));
            ASSERT_STR_EQ(row[0], "1");
            ASSERT_STR_EQ(row[1], "2");
            ASSERT_EQ(r.line(), 2);
            ASSERT_TRUE(r.next(row));
            ASSERT_STR_EQ(row[1], "4");                 // no trailing \r, no newline needed
            ASSERT_EQ(r.line(), 4);                     // the blank line still counts
            ASSERT_FALSE(r.next(row));
        }

        // A quoted field spanning lines is one record, numbered by its first line.
        writeFile(dir, name, "a,b\n1,\"x\ny\nz\"\n2,3\n");
        {
            CsvReader r(full, name, 2);
            ASSERT_TRUE(r.next(row));
            ASSERT_STR_EQ(row[1], "x\ny\nz");
            ASSERT_EQ(r.line(), 2);
            ASSERT_TRUE(r.next(row));
            ASSERT_EQ(r.line(), 5);
        }

        // Wrong header width, wrong row width, and an unclosed quote name the line.
        writeFile(dir, name, "a,b,c\n");
        ASSERT_THROWS(CsvReader(full, name, 2), FileFormatException);
        writeFile(dir, name, "a,b\n1,2,3\n");
        {
            CsvReader r(full, name, 2);
            ASSERT_THROWS(r.next(row), FileFormatException);
            ASSERT_EQ(r.line(), 2);
        }
        writeFile(dir, name, "a,b\n1,\"never closed\n");
        {
            CsvReader r(full, name, 2);
            bool caught = false;
            try {
                r.next(row);
            } catch (const FileFormatException& e) {
                caught = true;
                ASSERT_EQ(e.getLine(), 2);
            }
            ASSERT_TRUE(caught);
        }

        // Missing file: not open, nothing read.
        std::remove(full.c_str());
        CsvReader missing(full, name, 2);
        ASSERT_FALSE(missing.isOpen());
        ASSERT_FALSE(missing.next(row));
    }

    // A partial data directory loads what it has: a missing file is not an error.
    {
        saveWorld(store, good);
        std::remove((dir + "/shelters.csv").c_str());
        std::remove((dir + "/health.csv").c_str());
        World partial;
        loadWorld(store, partial);
        ASSERT_EQ(partial.camps.size(), 2);
        ASSERT_EQ(partial.camps.at(0)->shelterCount(), 0);
        ASSERT_EQ(partial.health.size(), 0);
    }

    // A missing data directory is simply "no data yet", not a crash.
    CsvStore absent("no_such_dir_xyz");
    ASSERT_FALSE(absent.dataExists());
    World none;
    loadWorld(absent, none);
    ASSERT_EQ(none.camps.size(), 0);
    // ... but saving somewhere that does not exist says so, rather than
    // silently losing the operator's data.
    ASSERT_THROWS(saveWorld(absent, good), FileFormatException);
    ASSERT_FALSE(absent.dataExists());

    removeDataFiles(dir);
    ASSERT_FALSE(store.dataExists());

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
