#include "CsvStore.h"
#include "Csv.h"
#include "CsvReader.h"
#include "Exceptions.h"
#include "Resident.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
#include "Debug.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <set>
#include <utility>
#include <cstdio>
#include <utility>

// ------------------------------------------------------------ file names
static const std::string kCampsFile = "camps.csv";
static const std::string kSheltersFile = "shelters.csv";
static const std::string kStockFile = "stock.csv";
static const std::string kFamiliesFile = "families.csv";
static const std::string kMembersFile = "members.csv";
static const std::string kVolunteersFile = "volunteers.csv";
static const std::string kDistributionsFile = "distributions.csv";
static const std::string kStockTxnsFile = "stock_txns.csv";
static const std::string kHealthFile = "health.csv";

// distributions.csv is familyId, campId, date, then one column per item.
static const int kDistributionFixedColumns = 3;

// ------------------------------------------------------------ writing
static std::string numberText(double value) {
    std::ostringstream oss;
    oss << std::setprecision(12) << value;
    return oss.str();
}

static std::string intText(int value) {
    return std::to_string(value);
}

static std::string flagText(bool value) {
    return value ? "1" : "0";
}

// Writes with "\n" in binary mode, so every platform produces the same bytes.
// The text goes to "<file>.tmp" first; saveAll renames it into place only once
// all nine files have been written, so a full disk or a crash mid-save leaves
// the previous files untouched.
static std::string tempPath(const std::string& fullPath) {
    return fullPath + ".tmp";
}

static void writeLines(const std::string& fullPath, const std::string& fileName,
                       const std::vector<std::string>& lines) {
    std::ofstream out(tempPath(fullPath).c_str(), std::ios::binary);
    if (!out.is_open()) {
        throw FileFormatException(fileName, 0, "cannot write " + tempPath(fullPath));
    }
    for (std::size_t i = 0; i < lines.size(); i++) {
        out << lines[i] << '\n';
    }
    out.close();
    if (out.fail()) {
        throw FileFormatException(fileName, 0, "error while writing " + tempPath(fullPath));
    }
}

// Moves the finished temporary file over the real one without ever having
// both copies absent: the old file is renamed to "<name>.bak" first and only
// deleted once the new one is in place. (std::rename will not overwrite on
// Windows, which is why a plain rename is not enough.)
static void commitFile(const std::string& fullPath, const std::string& fileName) {
    const std::string backup = fullPath + ".bak";
    std::remove(backup.c_str());                        // a stale one from a crash
    bool hadOld = false;
    {
        std::ifstream probe(fullPath.c_str());
        hadOld = probe.is_open();
    }
    if (hadOld && std::rename(fullPath.c_str(), backup.c_str()) != 0) {
        throw FileFormatException(fileName, 0, "cannot back up " + fullPath +
                                  "; the new data is kept in " + tempPath(fullPath));
    }
    if (std::rename(tempPath(fullPath).c_str(), fullPath.c_str()) != 0) {
        if (hadOld) {
            std::rename(backup.c_str(), fullPath.c_str());     // put the old file back
        }
        throw FileFormatException(fileName, 0, "cannot replace " + fullPath +
                                  "; the new data is kept in " + tempPath(fullPath));
    }
    if (hadOld) {
        std::remove(backup.c_str());
    }
}

// ------------------------------------------------------------ reading
static void fail(const std::string& file, int line, const std::string& detail) {
    throw FileFormatException(file, line, detail);
}

static bool parseFlag(const std::string& text, const std::string& file, int line) {
    if (text == "1") return true;
    if (text == "0") return false;
    fail(file, line, "expected 0 or 1, found \"" + text + "\"");
    return false;
}

// Every date read from a file goes through here. Date does not check itself,
// so an unchecked bad date would silently corrupt the duplicate-claim rule and
// the health monitor's 3-day window.
static Date parseDate(const std::string& text, const std::string& file, int line) {
    Date date;
    try {
        date = Date::parse(text);
    } catch (const std::invalid_argument&) {
        fail(file, line, "expected a date like 2026-07-14, found \"" + text + "\"");
    }
    if (!date.isValid()) {
        fail(file, line, "not a real calendar date: \"" + text + "\"");
    }
    return date;
}

static Item parseItem(const std::string& text, const std::string& file, int line) {
    try {
        return itemFromName(text);
    } catch (const std::invalid_argument&) {
        fail(file, line, "unknown item \"" + text + "\"");
    }
    return Item::Rice;      // not reached: fail() always throws
}

static Symptom parseSymptom(const std::string& text, const std::string& file, int line) {
    try {
        return symptomFromName(text);
    } catch (const std::invalid_argument&) {
        fail(file, line, "unknown symptom \"" + text + "\"");
    }
    return Symptom::Other;  // not reached
}

static char parseSex(const std::string& text, const std::string& file, int line) {
    if (text.size() == 1 && (text[0] == 'M' || text[0] == 'F' || text[0] == 'O')) {
        return text[0];
    }
    fail(file, line, "expected sex M, F or O, found \"" + text + "\"");
    return 'O';
}

static int parseId(const std::string& text, const std::string& file, int line) {
    int id = Csv::toInt(text, file, line);
    if (id <= 0) {
        fail(file, line, "id must be positive, found " + text);
    }
    return id;
}

static int parseNonNegative(const std::string& text, const std::string& what,
                            const std::string& file, int line) {
    int value = Csv::toInt(text, file, line);
    if (value < 0) {
        fail(file, line, what + " cannot be negative, found " + text);
    }
    return value;
}

static double parseQuantity(const std::string& text, const std::string& file, int line) {
    double value = Csv::toDouble(text, file, line);
    if (value < 0.0) {
        fail(file, line, "a quantity cannot be negative, found " + text);
    }
    return value;
}

// ------------------------------------------------------------ CsvStore
CsvStore::CsvStore(const std::string& dir) : dir(dir) {}

const std::string& CsvStore::getDir() const { return dir; }

std::string CsvStore::path(const std::string& fileName) const {
    return dir + "/" + fileName;
}

bool CsvStore::dataExists() const {
    std::ifstream in(path(kCampsFile).c_str());
    return in.is_open();
}

// ---- makePerson
Person* CsvStore::makePerson(const std::string& typeTag, const std::string& name,
                             int age, char sex, int familyId, int campId,
                             const std::string& skill, const std::string& shift) {
    if (typeTag == "INFANT") return new Infant(name, age, sex, familyId, campId);
    if (typeTag == "CHILD") return new Child(name, age, sex, familyId, campId);
    if (typeTag == "ADULT") return new Adult(name, age, sex, familyId, campId);
    if (typeTag == "ELDERLY") return new Elderly(name, age, sex, familyId, campId);
    if (typeTag == "RESIDENT_VOLUNTEER") {
        return new ResidentVolunteer(name, age, sex, familyId, campId, skill, shift);
    }
    if (typeTag == "VOLUNTEER") {
        // An outside volunteer needs an organisation this signature cannot
        // carry, and is never a family member: build it directly instead.
        throw std::invalid_argument("VOLUNTEER cannot be made here; construct a Volunteer directly");
    }
    throw std::invalid_argument("unknown person type \"" + typeTag + "\"");
}

// ---- save
void CsvStore::saveCamps(const Registry<Camp>& camps) const {
    std::vector<std::string> lines;
    lines.push_back("id,name,district,capacity,occupancy");
    for (int i = 0; i < camps.size(); i++) {
        const Camp* c = camps.at(i);
        std::vector<std::string> row;
        row.push_back(intText(c->getId()));
        row.push_back(c->getName());
        row.push_back(c->getDistrict());
        row.push_back(intText(c->getCapacity()));
        row.push_back(intText(c->getOccupancy()));
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kCampsFile), kCampsFile, lines);
}

void CsvStore::saveShelters(const Registry<Camp>& camps) const {
    std::vector<std::string> lines;
    lines.push_back("campId,label,capacity,occupancy");
    for (int i = 0; i < camps.size(); i++) {
        const Camp* c = camps.at(i);
        for (int s = 0; s < c->shelterCount(); s++) {
            const Camp::Shelter& sh = c->shelterAt(s);
            std::vector<std::string> row;
            row.push_back(intText(c->getId()));
            row.push_back(sh.getLabel());
            row.push_back(intText(sh.getCapacity()));
            row.push_back(intText(sh.getOccupancy()));
            lines.push_back(Csv::join(row));
        }
    }
    writeLines(path(kSheltersFile), kSheltersFile, lines);
}

void CsvStore::saveStock(const Registry<Camp>& camps) const {
    std::vector<std::string> lines;
    lines.push_back("campId,item,quantity");
    for (int i = 0; i < camps.size(); i++) {
        const Camp* c = camps.at(i);
        for (int k = 0; k < ITEM_COUNT; k++) {
            Item it = itemAt(k);
            std::vector<std::string> row;
            row.push_back(intText(c->getId()));
            row.push_back(itemName(it));
            row.push_back(numberText(c->getStock().get(it)));
            lines.push_back(Csv::join(row));
        }
    }
    writeLines(path(kStockFile), kStockFile, lines);
}

void CsvStore::saveFamilies(const Registry<Family>& families) const {
    std::vector<std::string> lines;
    lines.push_back("id,headName,village,campId");
    for (int i = 0; i < families.size(); i++) {
        const Family* f = families.at(i);
        std::vector<std::string> row;
        row.push_back(intText(f->getId()));
        row.push_back(f->getHeadName());
        row.push_back(f->getVillage());
        row.push_back(intText(f->getCampId()));
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kFamiliesFile), kFamiliesFile, lines);
}

void CsvStore::saveMembers(const Registry<Family>& families) const {
    std::vector<std::string> lines;
    lines.push_back("person_id,familyId,typeTag,name,age,sex,pregnant,disabled,lactating,skill,shift,shelterLabel");
    for (int i = 0; i < families.size(); i++) {
        const Family* f = families.at(i);
        for (int m = 0; m < f->size(); m++) {
            const Person& member = f->memberAt(m);
            std::string skill;
            std::string shift;
            std::string shelterLabel;
            const Resident* resident = dynamic_cast<const Resident*>(&member);
            if (resident != 0) {
                shelterLabel = resident->getShelterLabel();
            }
            // dynamic_cast is the honest tool here: only some members carry a skill.
            const ResidentVolunteer* worker = dynamic_cast<const ResidentVolunteer*>(&member);
            if (worker != 0) {
                skill = worker->getSkill();
                shift = worker->getShift();
            }
            std::vector<std::string> row;
            row.push_back(intText(member.getId()));
            row.push_back(intText(f->getId()));
            row.push_back(member.typeTag());
            row.push_back(member.getName());
            row.push_back(intText(member.getAge()));
            row.push_back(std::string(1, member.getSex()));
            row.push_back(flagText(member.isPregnant()));
            row.push_back(flagText(member.isDisabled()));
            row.push_back(flagText(member.isLactating()));
            row.push_back(skill);
            row.push_back(shift);
            row.push_back(shelterLabel);
            lines.push_back(Csv::join(row));
        }
    }
    writeLines(path(kMembersFile), kMembersFile, lines);
}

void CsvStore::saveVolunteers(const Registry<Volunteer>& volunteers) const {
    std::vector<std::string> lines;
    lines.push_back("id,name,age,sex,skill,shift,organisation");
    for (int i = 0; i < volunteers.size(); i++) {
        const Volunteer* v = volunteers.at(i);
        std::vector<std::string> row;
        row.push_back(intText(v->getId()));
        row.push_back(v->getName());
        row.push_back(intText(v->getAge()));
        row.push_back(std::string(1, v->getSex()));
        row.push_back(v->getSkill());
        row.push_back(v->getShift());
        row.push_back(v->getOrganisation());
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kVolunteersFile), kVolunteersFile, lines);
}

void CsvStore::saveDistributions(const DistributionLog& log) const {
    std::string header = "familyId,campId,date";
    for (int k = 0; k < ITEM_COUNT; k++) {
        header += "," + itemName(itemAt(k));
    }
    std::vector<std::string> lines;
    lines.push_back(header);
    for (int i = 0; i < log.size(); i++) {
        const Allocation& a = log.at(i);
        std::vector<std::string> row;
        row.push_back(intText(a.getFamilyId()));
        row.push_back(intText(a.getCampId()));
        row.push_back(a.getDate().toString());
        for (int k = 0; k < ITEM_COUNT; k++) {
            row.push_back(numberText(a.getAmount().get(itemAt(k))));
        }
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kDistributionsFile), kDistributionsFile, lines);
}

void CsvStore::saveStockTxns(const StockLedger& ledger) const {
    std::vector<std::string> lines;
    lines.push_back("campId,item,quantity,incoming,date,note");
    for (int i = 0; i < ledger.size(); i++) {
        const StockTxn& t = ledger.at(i);
        std::vector<std::string> row;
        row.push_back(intText(t.getCampId()));
        row.push_back(itemName(t.getItem()));
        row.push_back(numberText(t.getQuantity()));
        row.push_back(flagText(t.isIncoming()));
        row.push_back(t.getDate().toString());
        row.push_back(t.getNote());
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kStockTxnsFile), kStockTxnsFile, lines);
}

void CsvStore::saveHealth(const HealthMonitor& health) const {
    std::vector<std::string> lines;
    lines.push_back("campId,date,symptom,count");
    for (int i = 0; i < health.size(); i++) {
        const HealthCase& h = health.at(i);
        std::vector<std::string> row;
        row.push_back(intText(h.getCampId()));
        row.push_back(h.getDate().toString());
        row.push_back(symptomName(h.getSymptom()));
        row.push_back(intText(h.getCount()));
        lines.push_back(Csv::join(row));
    }
    writeLines(path(kHealthFile), kHealthFile, lines);
}

void CsvStore::saveAll(const Registry<Camp>& camps,
                       const Registry<Family>& families,
                       const Registry<Volunteer>& volunteers,
                       const DistributionLog& log,
                       const StockLedger& ledger,
                       const HealthMonitor& health) const {
    const std::string* names[] = {&kCampsFile, &kSheltersFile, &kStockFile, &kFamiliesFile,
                                  &kMembersFile, &kVolunteersFile, &kDistributionsFile,
                                  &kStockTxnsFile, &kHealthFile};
    const int fileCount = 9;
    int committing = -1;        // index of the file being renamed, if any
    try {
        // Step 1: write every file under a temporary name.
        saveCamps(camps);
        saveShelters(camps);
        saveStock(camps);
        saveFamilies(families);
        saveMembers(families);
        saveVolunteers(volunteers);
        saveDistributions(log);
        saveStockTxns(ledger);
        saveHealth(health);
        // Step 2: only now replace the real files.
        for (int i = 0; i < fileCount; i++) {
            committing = i;
            commitFile(path(*names[i]), *names[i]);
        }
    } catch (...) {
        for (int i = 0; i < fileCount; i++) {
            if (i != committing) {                               // no stray .tmp left behind
                std::remove(tempPath(path(*names[i])).c_str());
            }
        }
        // The .tmp of a failed commit is kept (the exception names it) so the
        // newest data stays recoverable.
        throw;
    }
}

// ---- load
// ID discipline for the three kinds of ID: adoptId gives a reloaded object its
// saved id, but it does NOT move the counter, so each loader finishes with
// resetIdCounter(largest loaded id + 1). Without that, the next new object
// would be handed an id that is already taken.

void CsvStore::loadCamps(Registry<Camp>& camps) const {
    CsvReader reader(path(kCampsFile), kCampsFile, 5);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    int maxId = 0;
    while (reader.next(f)) {
        int line = reader.line();
        int id = parseId(f[0], kCampsFile, line);
        if (camps.findById(id) != 0) {
            fail(kCampsFile, line, "duplicate camp id " + f[0]);
        }
        int capacity = parseNonNegative(f[3], "capacity", kCampsFile, line);
        int occupancy = parseNonNegative(f[4], "occupancy", kCampsFile, line);
        // Validate before calling the setter, so the operator sees the file
        // and line rather than a bare OverCapacityException.
        if (occupancy > capacity) {
            fail(kCampsFile, line, "occupancy " + f[4] + " is above capacity " + f[3]);
        }
        std::unique_ptr<Camp> camp(new Camp(f[1], f[2], capacity));
        camp->adoptId(id);
        camp->setOccupancy(occupancy);
        camps.add(std::move(camp));
        if (id > maxId) {
            maxId = id;
        }
    }
    // Never backwards past ids already handed out in this session.
    int current = Camp::generateId();
    Camp::resetIdCounter(current > maxId + 1 ? current : maxId + 1);
}

void CsvStore::loadShelters(Registry<Camp>& camps) const {
    CsvReader reader(path(kSheltersFile), kSheltersFile, 4);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    while (reader.next(f)) {
        int line = reader.line();
        int campId = Csv::toInt(f[0], kSheltersFile, line);
        Camp* camp = camps.findById(campId);
        if (camp == 0) {
            fail(kSheltersFile, line, "no camp with id " + f[0]);
        }
        int capacity = parseNonNegative(f[2], "capacity", kSheltersFile, line);
        int occupancy = parseNonNegative(f[3], "occupancy", kSheltersFile, line);
        if (occupancy > capacity) {
            fail(kSheltersFile, line, "occupancy " + f[3] + " is above capacity " + f[2]);
        }
        // Restored exactly as saved. The model does not keep a camp's head-count
        // and its shelters' figures in lockstep, so they are not cross-checked.
        Camp::Shelter shelter(f[1], capacity);
        shelter.setOccupancy(occupancy);
        camp->addShelter(shelter);
    }
}

void CsvStore::loadStock(Registry<Camp>& camps) const {
    CsvReader reader(path(kStockFile), kStockFile, 3);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    std::set<std::pair<int, int> > seen;      // (camp id, item index) already read
    while (reader.next(f)) {
        int line = reader.line();
        int campId = Csv::toInt(f[0], kStockFile, line);
        Camp* camp = camps.findById(campId);
        if (camp == 0) {
            fail(kStockFile, line, "no camp with id " + f[0]);
        }
        Item item = parseItem(f[1], kStockFile, line);
        if (!seen.insert(std::make_pair(campId, itemIndex(item))).second) {
            fail(kStockFile, line, "camp " + f[0] + " already has a stock row for " + f[1]);
        }
        double quantity = parseQuantity(f[2], kStockFile, line);   // Stock::set does not check
        camp->stockRef().set(item, quantity);
    }
}

void CsvStore::loadFamilies(const Registry<Camp>& camps, Registry<Family>& families) const {
    CsvReader reader(path(kFamiliesFile), kFamiliesFile, 4);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    int maxId = 0;
    while (reader.next(f)) {
        int line = reader.line();
        int id = parseId(f[0], kFamiliesFile, line);
        if (families.findById(id) != 0) {
            fail(kFamiliesFile, line, "duplicate family id " + f[0]);
        }
        int campId = parseNonNegative(f[3], "camp id", kFamiliesFile, line);
        if (campId != 0 && camps.findById(campId) == 0) {      // 0 means "not yet admitted"
            fail(kFamiliesFile, line, "no camp with id " + f[3]);
        }
        std::unique_ptr<Family> family(new Family(f[1], f[2]));
        family->adoptId(id);
        family->setCampId(campId);
        families.add(std::move(family));
        if (id > maxId) {
            maxId = id;
        }
    }
    int current = Family::generateId();
    Family::resetIdCounter(current > maxId + 1 ? current : maxId + 1);
}

void CsvStore::loadMembers(const Registry<Volunteer>& volunteers,
                           Registry<Family>& families) const {
    CsvReader reader(path(kMembersFile), kMembersFile, 12);
    if (!reader.isOpen()) {
        return;
    }
    // Volunteers and family members share one Person id space.
    std::set<int> usedIds;
    for (int i = 0; i < volunteers.size(); i++) {
        usedIds.insert(volunteers.at(i)->getId());
    }
    std::vector<std::string> f;
    int maxId = 0;
    while (reader.next(f)) {
        int line = reader.line();
        int personId = parseId(f[0], kMembersFile, line);
        if (usedIds.count(personId) != 0) {
            fail(kMembersFile, line, "duplicate person id " + f[0]);
        }
        int familyId = Csv::toInt(f[1], kMembersFile, line);
        Family* family = families.findById(familyId);
        if (family == 0) {
            fail(kMembersFile, line, "no family with id " + f[1]);
        }
        const std::string& tag = f[2];
        if (tag == "VOLUNTEER") {
            fail(kMembersFile, line, "VOLUNTEER belongs in volunteers.csv, not in a family");
        }
        int age = parseNonNegative(f[4], "age", kMembersFile, line);
        char sex = parseSex(f[5], kMembersFile, line);
        bool pregnant = parseFlag(f[6], kMembersFile, line);
        bool disabled = parseFlag(f[7], kMembersFile, line);
        bool lactating = parseFlag(f[8], kMembersFile, line);

        // Each member is built directly (never cloned: Person's copy
        // constructor would mint a new id), given its saved id, and handed to
        // the family, which takes ownership. Its camp is its family's camp.
        Person* person = 0;
        try {
            person = makePerson(tag, f[3], age, sex, familyId, family->getCampId(), f[9], f[10]);
        } catch (const std::invalid_argument&) {
            fail(kMembersFile, line, "unknown person type \"" + tag + "\"");
        }
        person->adoptId(personId);
        // A resident's own placement; empty means not placed in a shelter.
        Resident* resident = dynamic_cast<Resident*>(person);
        if (resident != 0 && !f[11].empty()) {
            resident->assignTo(family->getCampId(), f[11]);
        }
        person->setFlags(pregnant, disabled, lactating);
        family->addMember(person);
        usedIds.insert(personId);
        if (personId > maxId) {
            maxId = personId;
        }
    }
    // One past the largest id seen in members.csv AND volunteers.csv, and
    // never backwards past ids already handed out in this session.
    int next = Person::lastIssuedId() + 1;
    if (maxId + 1 > next) next = maxId + 1;
    for (std::set<int>::const_iterator it = usedIds.begin(); it != usedIds.end(); ++it) {
        if (*it + 1 > next) next = *it + 1;
    }
    Person::resetIdCounter(next);
}

void CsvStore::loadVolunteers(Registry<Volunteer>& volunteers) const {
    CsvReader reader(path(kVolunteersFile), kVolunteersFile, 7);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    int maxId = 0;
    while (reader.next(f)) {
        int line = reader.line();
        int id = parseId(f[0], kVolunteersFile, line);
        if (volunteers.findById(id) != 0) {
            fail(kVolunteersFile, line, "duplicate volunteer id " + f[0]);
        }
        int age = parseNonNegative(f[2], "age", kVolunteersFile, line);
        char sex = parseSex(f[3], kVolunteersFile, line);
        std::unique_ptr<Volunteer> v(new Volunteer(f[1], age, sex, f[4], f[5], f[6]));
        v->adoptId(id);
        volunteers.add(std::move(v));
        if (id > maxId) {
            maxId = id;
        }
    }
    // Family members share the Person id counter. Never move it backwards:
    // objects created earlier in this session already used ids below it.
    int next = Person::lastIssuedId() + 1;
    if (maxId + 1 > next) {
        next = maxId + 1;
    }
    Person::resetIdCounter(next);
}

void CsvStore::loadDistributions(DistributionLog& log) const {
    CsvReader reader(path(kDistributionsFile), kDistributionsFile,
                     kDistributionFixedColumns + ITEM_COUNT);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    while (reader.next(f)) {
        int line = reader.line();
        int familyId = parseId(f[0], kDistributionsFile, line);
        int campId = parseId(f[1], kDistributionsFile, line);
        Date date = parseDate(f[2], kDistributionsFile, line);
        Ration amount;
        for (int k = 0; k < ITEM_COUNT; k++) {
            amount.set(itemAt(k), parseQuantity(f[kDistributionFixedColumns + k],
                                                kDistributionsFile, line));
        }
        // loadEntry skips the duplicate rule (the file says what already
        // happened), so a hand-edited file is checked here instead: one claim
        // per family per day, whatever the camp.
        if (log.alreadyClaimed(familyId, date)) {
            fail(kDistributionsFile, line, "family " + f[0] + " already claimed on " + f[2]);
        }
        log.loadEntry(Allocation(familyId, campId, date, amount));
    }
}

void CsvStore::loadStockTxns(StockLedger& ledger) const {
    CsvReader reader(path(kStockTxnsFile), kStockTxnsFile, 6);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    while (reader.next(f)) {
        int line = reader.line();
        int campId = parseId(f[0], kStockTxnsFile, line);
        Item item = parseItem(f[1], kStockTxnsFile, line);
        double quantity = Csv::toDouble(f[2], kStockTxnsFile, line);
        if (quantity <= 0.0) {                       // loadTxn does not check
            fail(kStockTxnsFile, line, "a quantity must be above zero, found " + f[2]);
        }
        bool incoming = parseFlag(f[3], kStockTxnsFile, line);
        Date date = parseDate(f[4], kStockTxnsFile, line);
        ledger.loadTxn(StockTxn(campId, item, quantity, incoming, date, f[5]));
    }
}

void CsvStore::loadHealth(HealthMonitor& health) const {
    CsvReader reader(path(kHealthFile), kHealthFile, 4);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    while (reader.next(f)) {
        int line = reader.line();
        int campId = parseId(f[0], kHealthFile, line);
        Date date = parseDate(f[1], kHealthFile, line);
        Symptom symptom = parseSymptom(f[2], kHealthFile, line);
        int count = Csv::toInt(f[3], kHealthFile, line);
        if (count <= 0) {                            // loadCase does not check
            fail(kHealthFile, line, "a case count must be above zero, found " + f[3]);
        }
        health.loadCase(HealthCase(campId, date, symptom, count));
    }
}

// A camp's daily demand is what its families eat in a day. It is derived from
// the people, so it is recomputed here rather than stored.
static void restoreDemand(Registry<Camp>& camps, const Registry<Family>& families) {
    for (int i = 0; i < camps.size(); i++) {
        Camp* camp = camps.at(i);
        Ration total;
        for (int j = 0; j < families.size(); j++) {
            const Family* family = families.at(j);
            if (family->getCampId() == camp->getId()) {
                total += family->dailyRation();
            }
        }
        camp->stockRef().setDailyDemand(total);
    }
}

void CsvStore::loadAll(Registry<Camp>& camps,
                       Registry<Family>& families,
                       Registry<Volunteer>& volunteers,
                       DistributionLog& log,
                       StockLedger& ledger,
                       HealthMonitor& health) const {
    camps.clear();
    families.clear();
    volunteers.clear();
    log.clear();
    ledger.clear();
    health.clear();

    try {
        // Order: shelters and stock need their camps, families need the camps
        // they sit in, members need their families. Volunteers are loaded
        // before members; with saved ids on both sides that is no longer
        // required (each loader keeps the Person counter past every id seen),
        // it just keeps the checks simple.
        loadCamps(camps);
        loadShelters(camps);
        loadStock(camps);
        loadFamilies(camps, families);
        loadVolunteers(volunteers);
        loadMembers(volunteers, families);

        // stock.csv stores quantities only. A camp's daily demand is derived
        // data: it is recomputed here from the camp's families, exactly as the
        // menu's refreshDemand does. A hand-set demand that differs from that
        // sum will therefore not survive a reload.
        restoreDemand(camps, families);

        loadDistributions(log);
        loadStockTxns(ledger);
        loadHealth(health);
    } catch (...) {
        // A failed load must not leave half a camp behind: empty everything
        // and let the file-and-line message reach the operator unchanged.
        camps.clear();
        families.clear();
        volunteers.clear();
        log.clear();
        ledger.clear();
        health.clear();
        throw;
    }
    RELIEF_LIFECYCLE("Loaded " << camps.size() << " camps, " << families.size() << " families from " << dir);
}
