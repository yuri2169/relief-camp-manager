#include "Person.h"
#include "Debug.h"

// [Static data members] definitions live in exactly one .cpp file
int Person::nextId = 1;
int Person::liveCount = 0;

Person::Person()
    : id(generateId()), name("unnamed"), age(0), sex('O'),
      pregnant(false), disabled(false), lactating(false) {
    liveCount++;
    RELIEF_LIFECYCLE("Person created  #" << id << " " << name);
}

Person::Person(const std::string& name, int age, char sex)
    : id(generateId()), name(name), age(age), sex(sex),
      pregnant(false), disabled(false), lactating(false) {
    liveCount++;
    RELIEF_LIFECYCLE("Person created  #" << id << " " << this->name);
}

// Copies the data but takes a fresh ID: two people must never share an ID.
Person::Person(const Person& other)
    : id(generateId()), name(other.name), age(other.age), sex(other.sex),
      pregnant(other.pregnant), disabled(other.disabled),
      lactating(other.lactating) {
    liveCount++;
    RELIEF_LIFECYCLE("Person created  #" << id << " " << name);
}

Person::~Person() {
    liveCount--;
    RELIEF_LIFECYCLE("Person destroyed #" << id << " " << name);
}

int Person::generateId() { return nextId++; }
int Person::liveObjects() { return liveCount; }
void Person::resetIdCounter(int startAt) { nextId = startAt; }
int Person::lastIssuedId() { return nextId - 1; }

int Person::getId() const { return id; }
const std::string& Person::getName() const { return name; }
int Person::getAge() const { return age; }
char Person::getSex() const { return sex; }
bool Person::isPregnant() const { return pregnant; }
bool Person::isDisabled() const { return disabled; }
bool Person::isLactating() const { return lactating; }

void Person::setFlags(bool isPregnantNow, bool isDisabledNow, bool isLactatingNow) {
    pregnant = isPregnantNow;
    disabled = isDisabledNow;
    lactating = isLactatingNow;
}

void Person::setName(const std::string& newName) { name = newName; }
void Person::adoptId(int existingId) { id = existingId; }

int Person::flagBonus() const {
    return (pregnant ? 25 : 0) + (disabled ? 20 : 0) + (lactating ? 15 : 0);
}

std::ostream& operator<<(std::ostream& os, const Person& p) {
    os << "#" << p.id << " " << p.describe();
    return os;
}
