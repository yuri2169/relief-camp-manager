#ifndef RELIEF_RESIDENT_H
#define RELIEF_RESIDENT_H

#include "Person.h"
#include <string>

// A displaced person living in a camp. Still: a resident's ration
// depends on their age band, which the subclasses below supply.
//
// `virtual public Person` means a ResidentVolunteer, which
// inherits from both this branch and Volunteer, carries exactly ONE Person
// subobject. Without `virtual` it would have two IDs and two sets of
// vulnerability flags, and describe() would be ambiguous.
class Resident : virtual public Person {
    int familyId;
    int campId;
    std::string shelterLabel;
public:
    Resident();
    Resident(const std::string& name, int age, char sex, int familyId, int campId);
    Resident(const Resident& other);
    ~Resident() override;

    int getFamilyId() const;
    int getCampId() const;
    const std::string& getShelterLabel() const;
    void setFamilyId(int newFamilyId);
    void assignTo(int newCampId, const std::string& newShelterLabel);
};

#endif
