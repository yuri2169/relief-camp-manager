#include "Camp.h"
#include "Family.h"
#include "Exceptions.h"
#include "Debug.h"
#include <stdexcept>

// ---------------------------------------------------------------- Shelter

Camp::Shelter::Shelter() : label("unnamed"), capacity(0), occupancy(0) {}

Camp::Shelter::Shelter(const std::string& label, int capacity)
    : label(label), capacity(capacity), occupancy(0) {}

const std::string& Camp::Shelter::getLabel() const { return label; }
int Camp::Shelter::getCapacity() const { return capacity; }
int Camp::Shelter::getOccupancy() const { return occupancy; }
int Camp::Shelter::freeSpace() const { return capacity - occupancy; }
bool Camp::Shelter::hasRoom(int people) const { return people <= freeSpace(); }

void Camp::Shelter::occupy(int people) {
    if (people < 0) throw std::invalid_argument("cannot occupy a negative number of people");
    // The shelter does not know its camp's id, so it reports -1; Camp::admit
    // is the path that reports a real camp id.
    if (people > freeSpace()) throw OverCapacityException(-1, people, freeSpace());
    occupancy += people;
}

void Camp::Shelter::vacate(int people) {
    if (people < 0) throw std::invalid_argument("cannot vacate a negative number of people");
    occupancy -= people;
    if (occupancy < 0) occupancy = 0;
}

void Camp::Shelter::setOccupancy(int people) {
    // Check first, change second; CSV loading may feed this untrusted numbers.
    if (people < 0) throw std::invalid_argument("occupancy cannot be negative");
    if (people > capacity) throw OverCapacityException(-1, people, capacity);
    occupancy = people;
}

std::ostream& operator<<(std::ostream& os, const Camp::Shelter& s) {
    os << s.label << " [" << s.occupancy << "/" << s.capacity << "]";
    return os;
}

// ------------------------------------------------------------------ Camp

int Camp::nextId = 1;

Camp::Camp()
    : id(generateId()), name("unnamed"), district("unknown"),
      capacity(0), occupancy(0), shelters(), stock() {
    RELIEF_LIFECYCLE("Camp #" << id << " created");
}

Camp::Camp(const std::string& name, const std::string& district, int capacity)
    : id(generateId()), name(name), district(district),
      capacity(capacity), occupancy(0), shelters(), stock() {
    RELIEF_LIFECYCLE("Camp #" << id << " created");
}

// Camp needs no hand-written rule of three: it owns no raw memory. The vector
// and the Stock copy themselves. That is the contrast with Family.
Camp::Camp(const Camp& other)
    : id(other.id), name(other.name), district(other.district),
      capacity(other.capacity), occupancy(other.occupancy),
      shelters(other.shelters), stock(other.stock) {
    RELIEF_LIFECYCLE("Camp #" << id << " copied");
}

Camp::~Camp() {
    RELIEF_LIFECYCLE("Camp #" << id << " destroyed");
}

int Camp::generateId() { return nextId++; }
void Camp::resetIdCounter(int startAt) { nextId = startAt; }
void Camp::adoptId(int existingId) {
    id = existingId;
}

int Camp::getId() const { return id; }
const std::string& Camp::getName() const { return name; }
const std::string& Camp::getDistrict() const { return district; }
int Camp::getCapacity() const { return capacity; }
int Camp::getOccupancy() const { return occupancy; }
int Camp::freeSpace() const { return capacity - occupancy; }
bool Camp::isOverflowing() const { return occupancy >= capacity; }

double Camp::occupancyRate() const {
    return capacity == 0 ? 0.0 : static_cast<double>(occupancy) / capacity;
}

void Camp::admit(const Family& f) {
    int n = f.size();
    if (n == 0) return;
    // Check first, change second: a refused admission leaves the camp as it was.
    if (n > freeSpace()) throw OverCapacityException(id, n, freeSpace());
    occupancy += n;
}

void Camp::release(const Family& f) {
    occupancy -= f.size();
    if (occupancy < 0) occupancy = 0;
}

Camp& Camp::operator++() {
    if (occupancy >= capacity) throw OverCapacityException(id, 1, freeSpace());
    occupancy++;
    return *this;
}

Camp Camp::operator++(int) {
    Camp before(*this);
    ++(*this);              // the prefix form does the work and the guard
    return before;
}

Camp& Camp::operator--() {
    if (occupancy > 0) occupancy--;
    return *this;
}

Camp Camp::operator--(int) {
    Camp before(*this);
    --(*this);
    return before;
}

void Camp::addShelter(const Shelter& s) { shelters.push_back(s); }
int Camp::findShelterFor(int people) const {
    for (int i = 0; i < shelterCount(); i++) {
        if (shelters[i].hasRoom(people)) return i;
    }
    return -1;
}

int Camp::shelterCount() const { return static_cast<int>(shelters.size()); }

const Camp::Shelter& Camp::shelterAt(int index) const {
    if (index < 0 || index >= shelterCount()) {
        throw std::out_of_range("shelter index out of range");
    }
    return shelters[index];
}

Camp::Shelter& Camp::shelterAt(int index) {
    if (index < 0 || index >= shelterCount()) {
        throw std::out_of_range("shelter index out of range");
    }
    return shelters[index];
}

Stock& Camp::stockRef() { return stock; }
const Stock& Camp::getStock() const { return stock; }
void Camp::setOccupancy(int people) {
    if (people < 0) throw std::invalid_argument("occupancy cannot be negative");
    if (people > capacity) throw OverCapacityException(id, people, capacity);
    occupancy = people;
}
void Camp::setName(const std::string& newName) { name = newName; }

std::ostream& operator<<(std::ostream& os, const Camp& c) {
    os << "Camp #" << c.id << " " << c.name << ", " << c.district
       << " [" << c.occupancy << "/" << c.capacity << "]"
       << (c.isOverflowing() ? " OVERFLOW" : "");
    return os;
}
