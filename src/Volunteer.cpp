#include "Volunteer.h"
#include "RationNorms.h"
#include "Debug.h"

Volunteer::Volunteer()
    : Person(), skill(""), shift(""), organisation("") {
    RELIEF_LIFECYCLE("Volunteer created #" << getId());
}

// The Person(...) call here is used only when Volunteer is the most-derived class. For a ResidentVolunteer it is ignored, because a virtual base is initialised by the most-derived constructor.
Volunteer::Volunteer(const std::string& name, int age, char sex,
                     const std::string& skill, const std::string& shift,
                     const std::string& organisation)
    : Person(name, age, sex), skill(skill), shift(shift), organisation(organisation) {
    RELIEF_LIFECYCLE("Volunteer created #" << getId());
}

Volunteer::Volunteer(const Volunteer& other)
    : Person(other), skill(other.skill), shift(other.shift),
      organisation(other.organisation) {
    RELIEF_LIFECYCLE("Volunteer copied  #" << getId());
}

Volunteer::~Volunteer() {
    RELIEF_LIFECYCLE("Volunteer destroyed #" << getId());
}

const std::string& Volunteer::getSkill() const { return skill; }
const std::string& Volunteer::getShift() const { return shift; }
const std::string& Volunteer::getOrganisation() const { return organisation; }

void Volunteer::reassign(const std::string& newSkill, const std::string& newShift) {
    skill = newSkill;
    shift = newShift;
}

Ration Volunteer::rationNeed() const { return RationNorms::forVolunteer(); }
int Volunteer::vulnerabilityScore() const { return 0 + flagBonus(); }

std::string Volunteer::describe() const {
    return getName() + ", volunteer (" + getSkill() + ", " + getShift() +
           " shift, " + getOrganisation() + ")";
}

Person* Volunteer::clone() const { return new Volunteer(*this); }
std::string Volunteer::typeTag() const { return "VOLUNTEER"; }
