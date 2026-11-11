#include "Family.h"
#include "RationCard.h"
#include "Debug.h"
#include <stdexcept>

int Family::nextId = 1;

Family::Family()
    : id(generateId()), headName("unnamed"), village("unknown"), campId(0),
      members(0), count(0), capacity(0) {
    RELIEF_LIFECYCLE("Family created   #" << id);
}

Family::Family(const std::string& headName, const std::string& village)
    : id(generateId()), headName(headName), village(village), campId(0),
      members(0), count(0), capacity(0) {
    RELIEF_LIFECYCLE("Family created   #" << id);
}

// Note: the copy keeps the family id (it is the same family on paper), but
// Person's copy constructor mints fresh ids, so the cloned members are renumbered.
// Task 19's CSV loading uses adoptId on construction rather than cloning.
Family::Family(const Family& other)
    : id(other.id), headName(other.headName), village(other.village),
      campId(other.campId), members(0), count(0), capacity(0) {
    // [Copy constructor]
    // A shallow copy would leave two families pointing at the same Person
    // objects, and the second destructor would delete them twice. clone() gives
    // each family its own people.
    copyFrom(other);
    RELIEF_LIFECYCLE("Family copied    #" << id << " with " << count << " members");
}

Family& Family::operator=(const Family& other) {
    // [Copy assignment operator]
    // Guard against self-assignment FIRST. Without this, `f = f` would free its
    // own members and then copy from freed memory.
    if (this == &other) {
        return *this;
    }
    freeAll();
    id = other.id;
    headName = other.headName;
    village = other.village;
    campId = other.campId;
    copyFrom(other);
    return *this;          // returning a reference is what allows a = b = c
}

Family::~Family() {
    // [Destructor]
    // Family allocated the members, so Family frees them.
    RELIEF_LIFECYCLE("Family destroyed #" << id << " with " << count << " members");
    freeAll();
}

void Family::copyFrom(const Family& other) {
    int newCapacity = other.count > 0 ? other.count : 4;
    // [Dynamic memory allocation]
    // Allocate first and commit to the members only once it has succeeded, so a
    // failed allocation can never leave capacity set while members is null.
    Person** fresh = new Person*[newCapacity];
    int n = 0;
    for (int i = 0; i < other.count; i++) {
        fresh[n] = other.members[i]->clone();   // deep copy, keeps the type
        n++;
    }
    members = fresh;
    count = n;
    capacity = newCapacity;
}

void Family::freeAll() {
    for (int i = 0; i < count; i++) {
        delete members[i];
    }
    delete[] members;
    members = 0;
    count = 0;
    capacity = 0;
}

void Family::grow() {
    int newCapacity = capacity == 0 ? 4 : capacity * 2;
    // [Dynamic memory allocation]
    Person** bigger = new Person*[newCapacity];
    for (int i = 0; i < count; i++) {
        bigger[i] = members[i];
    }
    delete[] members;          // free the old array, not the people in it
    members = bigger;
    capacity = newCapacity;
}

void Family::addMember(Person* p) {
    if (p == 0) { return; }
    if (count == capacity) { grow(); }
    members[count] = p;
    count++;
}

int Family::generateId() {
    return nextId++;
}

void Family::resetIdCounter(int startAt) {
    nextId = startAt;
}

void Family::adoptId(int existingId) {
    id = existingId;
}

int Family::getId() const { return id; }
const std::string& Family::getHeadName() const { return headName; }
const std::string& Family::getVillage() const { return village; }
int Family::getCampId() const { return campId; }
void Family::setCampId(int newCampId) { campId = newCampId; }
void Family::setHeadName(const std::string& newHeadName) { headName = newHeadName; }

int Family::size() const { return count; }

const Person& Family::memberAt(int index) const {
    if (index < 0 || index >= count) {
        throw std::out_of_range("member index out of range");
    }
    return *members[index];
}

Person& Family::memberAt(int index) {
    if (index < 0 || index >= count) {
        throw std::out_of_range("member index out of range");
    }
    return *members[index];
}

int Family::countOfTag(const std::string& tag) const {
    int n = 0;
    for (int i = 0; i < count; i++) {
        if (members[i]->typeTag() == tag) { n++; }
    }
    return n;
}

Ration Family::dailyRation() const {
    Ration total;
    for (int i = 0; i < count; i++) {
        total += members[i]->rationNeed();
    }
    return total;
}

int Family::vulnerabilityScore() const {
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += members[i]->vulnerabilityScore();
    }
    return total;
}

double Family::metricVulnerability() const { return vulnerabilityScore(); }
double Family::metricSize() const { return count; }

double Family::metricAverageAge() const {
    if (count == 0) { return 0.0; }
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += members[i]->getAge();
    }
    return static_cast<double>(total) / count;
}

double Family::metricInfantCount() const { return countOfTag("INFANT"); }

bool operator<(const Family& a, const Family& b) {
    return a.vulnerabilityScore() < b.vulnerabilityScore();
}

bool operator==(const Family& a, const Family& b) {
    return a.id == b.id;
}

std::ostream& operator<<(std::ostream& os, const Family& f) {
    os << "Family #" << f.id << " " << f.headName << " (" << f.village << "), "
       << f.count << " members, vulnerability " << f.vulnerabilityScore();
    return os;
}

Family::operator RationCard() const {
    return RationCard(id, headName, campId, count,
                      countOfTag("INFANT"), countOfTag("CHILD"),
                      countOfTag("ADULT") + countOfTag("RESIDENT_VOLUNTEER"),
                      countOfTag("ELDERLY"),
                      dailyRation(), vulnerabilityScore());
}
