#include "ResidentVolunteer.h"
#include "Debug.h"

ResidentVolunteer::ResidentVolunteer()
    // [Virtual base initialisation]
    // The MOST-DERIVED class initialises the virtual base. Person(...) appears
    // here, not in Adult's or Volunteer's initialiser list, and their own
    // Person(...) calls are ignored for this object. That is exactly why there
    // is only one Person subobject.
    : Person(), Adult(), Volunteer() {
    RELIEF_LIFECYCLE("ResidentVolunteer created  #" << getId());
}

ResidentVolunteer::ResidentVolunteer(const std::string& name, int age, char sex,
                                     int familyId, int campId,
                                     const std::string& skill,
                                     const std::string& shift)
    // [Virtual base initialisation]
    // Person is named FIRST and initialised once.
    : Person(name, age, sex),
      Adult(name, age, sex, familyId, campId),
      Volunteer(name, age, sex, skill, shift, "camp resident") {
    RELIEF_LIFECYCLE("ResidentVolunteer created  #" << getId() << " " << name);
}

ResidentVolunteer::ResidentVolunteer(const ResidentVolunteer& other)
    : Person(other), Adult(other), Volunteer(other) {
    RELIEF_LIFECYCLE("ResidentVolunteer copied   #" << getId());
}

ResidentVolunteer::~ResidentVolunteer() {
    RELIEF_LIFECYCLE("ResidentVolunteer destroyed #" << getId());
}

// [Ambiguity resolution]
// Adult and Volunteer both supply these functions, so an unqualified call would
// be ambiguous. A resident volunteer eats with their family, so the Adult
// answer is the right one.
Ration ResidentVolunteer::rationNeed() const { return Adult::rationNeed(); }

int ResidentVolunteer::vulnerabilityScore() const { return Adult::vulnerabilityScore(); }

std::string ResidentVolunteer::describe() const {
    return getName() + ", resident volunteer (" + getSkill() + ", " + getShift()
         + " shift), family " + std::to_string(getFamilyId());
}

Person* ResidentVolunteer::clone() const { return new ResidentVolunteer(*this); }

std::string ResidentVolunteer::typeTag() const { return "RESIDENT_VOLUNTEER"; }
