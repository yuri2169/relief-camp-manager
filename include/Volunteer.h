#ifndef RELIEF_VOLUNTEER_H
#define RELIEF_VOLUNTEER_H

#include "Person.h"
#include <string>

// Someone working at the camp: a medic, a cook, a record keeper. Volunteers
// draw a worker meal, not a family ration.
//
// [Virtual base class] see the note in Resident.h - this is the other half of
// the diamond that ResidentVolunteer closes.
class Volunteer : virtual public Person {
    std::string skill;
    std::string shift;
    std::string organisation;
public:
    Volunteer();
    Volunteer(const std::string& name, int age, char sex,
              const std::string& skill, const std::string& shift,
              const std::string& organisation);
    Volunteer(const Volunteer& other);
    ~Volunteer() override;

    const std::string& getSkill() const;
    const std::string& getShift() const;
    const std::string& getOrganisation() const;
    void reassign(const std::string& newSkill, const std::string& newShift);

    // [Runtime polymorphism] overrides of the pure virtuals
    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
