#ifndef RELIEF_RESIDENTVOLUNTEER_H
#define RELIEF_RESIDENTVOLUNTEER_H

#include "Adult.h"
#include "Volunteer.h"

// A displaced adult who also works a shift at the camp. Very common in Assam
// camps, where residents run the kitchen and keep the registers.
//
// [Multiple inheritance]
// Inherits from Adult and Volunteer at once.
// [Hybrid inheritance]
// The whole tree combines hierarchical (Infant/Child/Adult from Resident),
// multilevel (Elderly from Adult) and multiple (this class) inheritance.
//
// Because Person is a VIRTUAL base of both Resident and Volunteer, this object
// holds exactly one Person subobject: one ID, one name, one set of flags. Its
// constructor must therefore initialise Person itself - see the .cpp.
class ResidentVolunteer : public Adult, public Volunteer {
public:
    ResidentVolunteer();
    ResidentVolunteer(const std::string& name, int age, char sex,
                      int familyId, int campId,
                      const std::string& skill, const std::string& shift);
    ResidentVolunteer(const ResidentVolunteer& other);
    ~ResidentVolunteer() override;

    // Both parents offer these, so the override must say which one wins.
    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
