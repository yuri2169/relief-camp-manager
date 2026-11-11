#include "CsvStore.h"
#include "Csv.h"
#include "Exceptions.h"
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
static void writeLines(const std::string& fullPath, const std::vector<std::string>& lines) {
    std::ofstream out(fullPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        throw ReliefException("cannot write " + fullPath);
    }
    for (std::size_t i = 0; i < lines.size(); i++) {
        out << lines[i] << '\n';
    }
    out.close();
    if (out.fail()) {
        throw ReliefException("error while writing " + fullPath);
    }
}

// ------------------------------------------------------------ reading
static void fail(const std::string& file, int line, const std::string& detail) {
    throw FileFormatException(file, line, detail);
}

// Reads one CSV file record by record. The header is line 1; the first data
// row is line 2. A quoted field may contain a line break, so one record can
// cover several physical lines; its number is the line it STARTS on. Blank
// lines are skipped, a trailing '\r' (a file saved on Windows) is ignored and
// so is a spreadsheet's byte-order mark.
class CsvReader {
    std::ifstream in;
    std::string file;
    int expected;       // columns every row must have
    int nextLine;       // number of the next physical line to read
    int recordLine;     // line the most recent record started on

    bool readPhysicalLine(std::string& text) {
        if (!std::getline(in, text)) {
            return false;
        }
        nextLine++;
        if (!text.empty() && text[text.size() - 1] == '\r') {
            text.erase(text.size() - 1);
        }
        return true;
    }

    static bool hasOpenQuote(const std::string& text) {
        bool open = false;
        for (std::size_t i = 0; i < text.size(); i++) {
            if (text[i] == '"') {
                open = !open;
            }
        }
        return open;
    }

    bool readRecord(std::vector<std::string>& fields) {
        std::string text;
        do {
            recordLine = nextLine;
            if (!readPhysicalLine(text)) {
                return false;
            }
        } while (text.empty());                     // skip blank lines
        if (recordLine == 1 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            text.erase(0, 3);
        }
        while (hasOpenQuote(text)) {                // the field continues on the next line
            std::string more;
            if (!readPhysicalLine(more)) {
                fail(file, recordLine, "a quoted field is never closed");
            }
            text += "\n" + more;
        }
        fields = Csv::split(text);
        return true;
    }

public:
    // Opens the file and checks the header has `columns` columns.
    CsvReader(const std::string& fullPath, const std::string& fileName, int columns)
        : in(fullPath.c_str(), std::ios::binary), file(fileName), expected(columns),
          nextLine(1), recordLine(0) {
        std::vector<std::string> header;
        if (in.is_open() && readRecord(header)) {
            Csv::require(static_cast<int>(header.size()), columns, file, recordLine);
        }
    }

    bool isOpen() const { return in.is_open(); }
    int line() const { return recordLine; }

    // The next data row, already checked to have the right number of fields.
    bool next(std::vector<std::string>& fields) {
        if (!readRecord(fields)) {
            return false;
        }
        Csv::require(static_cast<int>(fields.size()), expected, file, recordLine);
        return true;
    }
};

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
        return new Volunteer(name, age, sex, skill, shift, "");
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
    writeLines(path(kCampsFile), lines);
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
    writeLines(path(kSheltersFile), lines);
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
    writeLines(path(kStockFile), lines);
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
    writeLines(path(kFamiliesFile), lines);
}

void CsvStore::saveMembers(const Registry<Family>& families) const {
    std::vector<std::string> lines;
    lines.push_back("familyId,typeTag,name,age,sex,pregnant,disabled,lactating,skill,shift");
    for (int i = 0; i < families.size(); i++) {
        const Family* f = families.at(i);
        for (int m = 0; m < f->size(); m++) {
            const Person& member = f->memberAt(m);
            std::string skill;
            std::string shift;
            // dynamic_cast is the honest tool here: only some members carry a skill.
            const ResidentVolunteer* worker = dynamic_cast<const ResidentVolunteer*>(&member);
            if (worker != 0) {
                skill = worker->getSkill();
                shift = worker->getShift();
            }
            std::vector<std::string> row;
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
            lines.push_back(Csv::join(row));
        }
    }
    writeLines(path(kMembersFile), lines);
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
    writeLines(path(kVolunteersFile), lines);
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
    writeLines(path(kDistributionsFile), lines);
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
    writeLines(path(kStockTxnsFile), lines);
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
    writeLines(path(kHealthFile), lines);
}

void CsvStore::saveAll(const Registry<Camp>& camps,
                       const Registry<Family>& families,
                       const Registry<Volunteer>& volunteers,
                       const DistributionLog& log,
                       const StockLedger& ledger,
                       const HealthMonitor& health) const {
    saveCamps(camps);
    saveShelters(camps);
    saveStock(camps);
    saveFamilies(families);
    saveMembers(families);
    saveVolunteers(volunteers);
    saveDistributions(log);
    saveStockTxns(ledger);
    saveHealth(health);
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
    Camp::resetIdCounter(maxId + 1);
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
        // The camp's head-count is authoritative; its shelters only say where
        // those people sleep, so together they can never hold more than the camp.
        int placed = occupancy;
        for (int s = 0; s < camp->shelterCount(); s++) {
            placed += camp->shelterAt(s).getOccupancy();
        }
        if (placed > camp->getOccupancy()) {
            fail(kSheltersFile, line, "shelters of camp " + f[0] + " would hold " +
                 intText(placed) + " people but the camp holds only " +
                 intText(camp->getOccupancy()));
        }
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
    while (reader.next(f)) {
        int line = reader.line();
        int campId = Csv::toInt(f[0], kStockFile, line);
        Camp* camp = camps.findById(campId);
        if (camp == 0) {
            fail(kStockFile, line, "no camp with id " + f[0]);
        }
        Item item = parseItem(f[1], kStockFile, line);
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
    Family::resetIdCounter(maxId + 1);
}

void CsvStore::loadMembers(Registry<Family>& families) const {
    CsvReader reader(path(kMembersFile), kMembersFile, 10);
    if (!reader.isOpen()) {
        return;
    }
    std::vector<std::string> f;
    while (reader.next(f)) {
        int line = reader.line();
        int familyId = Csv::toInt(f[0], kMembersFile, line);
        Family* family = families.findById(familyId);
        if (family == 0) {
            fail(kMembersFile, line, "no family with id " + f[0]);
        }
        const std::string& tag = f[1];
        if (tag == "VOLUNTEER") {
            fail(kMembersFile, line, "VOLUNTEER belongs in volunteers.csv, not in a family");
        }
        int age = parseNonNegative(f[3], "age", kMembersFile, line);
        char sex = parseSex(f[4], kMembersFile, line);
        bool pregnant = parseFlag(f[5], kMembersFile, line);
        bool disabled = parseFlag(f[6], kMembersFile, line);
        bool lactating = parseFlag(f[7], kMembersFile, line);

        // Each member is built directly (never cloned: Person's copy
        // constructor would mint a new id) and the family takes ownership. Its
        // camp is its family's camp. Members have no saved id, so they take
        // fresh Person ids, which loadVolunteers has already kept clear of
        // every outside volunteer's id.
        Person* person = 0;
        try {
            person = makePerson(tag, f[2], age, sex, familyId, family->getCampId(), f[8], f[9]);
        } catch (const std::invalid_argument&) {
            fail(kMembersFile, line, "unknown person type \"" + tag + "\"");
        }
        person->setFlags(pregnant, disabled, lactating);
        family->addMember(person);
    }
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
        int familyId = Csv::toInt(f[0], kDistributionsFile, line);
        int campId = Csv::toInt(f[1], kDistributionsFile, line);
        Date date = parseDate(f[2], kDistributionsFile, line);
        Ration amount;
        for (int k = 0; k < ITEM_COUNT; k++) {
            amount.set(itemAt(k), parseQuantity(f[kDistributionFixedColumns + k],
                                                kDistributionsFile, line));
        }
        // loadEntry, not record: the file says what already happened.
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
        int campId = Csv::toInt(f[0], kStockTxnsFile, line);
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
        int campId = Csv::toInt(f[0], kHealthFile, line);
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

    // Order matters: shelters and stock need their camps, families need the
    // camps they sit in, members need their families. Volunteers go before
    // members so the Person id counter is already clear of their saved ids
    // when the members take fresh ones.
    loadCamps(camps);
    loadShelters(camps);
    loadStock(camps);
    loadFamilies(camps, families);
    loadVolunteers(volunteers);
    loadMembers(families);
    restoreDemand(camps, families);
    loadDistributions(log);
    loadStockTxns(ledger);
    loadHealth(health);
    RELIEF_LIFECYCLE("Loaded " << camps.size() << " camps, " << families.size() << " families from " << dir);
}
