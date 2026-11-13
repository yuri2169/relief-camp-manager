#include "Child.h"
#include "RationNorms.h"
#include "Debug.h"

Child::Child()
    : Person(), Resident() {
    RELIEF_LIFECYCLE("Child created #" << getId());
}

// Both bases are named: Person because it is a virtual base, so the most-derived class initialises it, and Resident for the family and camp data.
Child::Child(const std::string& name, int age, char sex, int familyId, int campId)
    : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {
    RELIEF_LIFECYCLE("Child created #" << getId());
}

Child::Child(const Child& other)
    : Person(other), Resident(other) {
    RELIEF_LIFECYCLE("Child copied  #" << getId());
}

Child::~Child() {
    RELIEF_LIFECYCLE("Child destroyed #" << getId());
}

Ration Child::rationNeed() const { return RationNorms::forChild(); }
int Child::vulnerabilityScore() const { return 20 + flagBonus(); }

std::string Child::describe() const {
    return getName() + ", child (" + std::to_string(getAge()) + "y), vulnerability " +
           std::to_string(vulnerabilityScore());
}

Person* Child::clone() const { return new Child(*this); }
std::string Child::typeTag() const { return "CHILD"; }
