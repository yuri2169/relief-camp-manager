#include "Infant.h"
#include "RationNorms.h"
#include "Debug.h"

Infant::Infant()
    : Person(), Resident() {
    RELIEF_LIFECYCLE("Infant created #" << getId());
}

// Both bases are named: Person because it is a virtual base, so the most-derived class initialises it, and Resident for the family and camp data. [Virtual base initialisation]
Infant::Infant(const std::string& name, int age, char sex, int familyId, int campId)
    : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {
    RELIEF_LIFECYCLE("Infant created #" << getId());
}

Infant::Infant(const Infant& other)
    : Person(other), Resident(other) {
    RELIEF_LIFECYCLE("Infant copied  #" << getId());
}

Infant::~Infant() {
    RELIEF_LIFECYCLE("Infant destroyed #" << getId());
}

Ration Infant::rationNeed() const { return RationNorms::forInfant(); }
int Infant::vulnerabilityScore() const { return 40 + flagBonus(); }

std::string Infant::describe() const {
    return getName() + ", infant (" + std::to_string(getAge()) + "y), vulnerability " +
           std::to_string(vulnerabilityScore());
}

Person* Infant::clone() const { return new Infant(*this); }
std::string Infant::typeTag() const { return "INFANT"; }
