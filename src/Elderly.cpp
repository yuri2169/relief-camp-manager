#include "Elderly.h"
#include "RationNorms.h"
#include "Debug.h"

Elderly::Elderly()
    : Person(), Adult() {
    RELIEF_LIFECYCLE("Elderly created #" << getId());
}

Elderly::Elderly(const std::string& name, int age, char sex, int familyId, int campId)
    : Person(name, age, sex), Adult(name, age, sex, familyId, campId) {
    RELIEF_LIFECYCLE("Elderly created #" << getId());
}

Elderly::Elderly(const Elderly& other)
    : Person(other), Adult(other) {
    RELIEF_LIFECYCLE("Elderly copied  #" << getId());
}

Elderly::~Elderly() {
    RELIEF_LIFECYCLE("Elderly destroyed #" << getId());
}

Ration Elderly::rationNeed() const {
    Ration r = RationNorms::forElderly();
    if (isPregnant()) r.add(Item::Rice, RationNorms::pregnancyRiceBonus());
    if (isLactating()) r.add(Item::Oil, RationNorms::lactationOilBonus());
    return r;
}

int Elderly::vulnerabilityScore() const { return 35 + flagBonus(); }

std::string Elderly::describe() const {
    return getName() + ", elderly (" + std::to_string(getAge()) + "y), vulnerability " +
           std::to_string(vulnerabilityScore());
}

Person* Elderly::clone() const { return new Elderly(*this); }
std::string Elderly::typeTag() const { return "ELDERLY"; }
