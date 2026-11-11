#include "Adult.h"
#include "RationNorms.h"
#include "Debug.h"

Adult::Adult()
    : Person(), Resident() {
    RELIEF_LIFECYCLE("Adult created #" << getId());
}

// Both bases are named: Person because it is a virtual base, so the most-derived class initialises it, and Resident for the family and camp data. [Virtual base initialisation]
Adult::Adult(const std::string& name, int age, char sex, int familyId, int campId)
    : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {
    RELIEF_LIFECYCLE("Adult created #" << getId());
}

Adult::Adult(const Adult& other)
    : Person(other), Resident(other) {
    RELIEF_LIFECYCLE("Adult copied  #" << getId());
}

Adult::~Adult() {
    RELIEF_LIFECYCLE("Adult destroyed #" << getId());
}

Ration Adult::rationNeed() const {
    Ration r = RationNorms::forAdult();
    if (isPregnant()) r.add(Item::Rice, RationNorms::pregnancyRiceBonus());
    if (isLactating()) r.add(Item::Oil, RationNorms::lactationOilBonus());
    // [Returning objects] the ration is built and returned by value
    return r;
}
int Adult::vulnerabilityScore() const { return 10 + flagBonus(); }

std::string Adult::describe() const {
    return getName() + ", adult (" + std::to_string(getAge()) + "y), vulnerability " +
           std::to_string(vulnerabilityScore());
}

Person* Adult::clone() const { return new Adult(*this); }
std::string Adult::typeTag() const { return "ADULT"; }
