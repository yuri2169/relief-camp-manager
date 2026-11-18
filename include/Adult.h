#ifndef RELIEF_ADULT_H
#define RELIEF_ADULT_H

#include "Resident.h"

// A camp resident aged 13-59, drawing the full adult
// ration. Pregnancy and lactation add to it.
// Infant, Child and Adult all derive from Resident.
// Adult is also the base of Elderly and of ResidentVolunteer.
class Adult : public Resident {
public:
    Adult();
    Adult(const std::string& name, int age, char sex, int familyId, int campId);
    Adult(const Adult& other);
    ~Adult() override;

    // Still virtual, so Elderly can override again
    virtual Ration rationNeed() const override;
    virtual int vulnerabilityScore() const override;
    virtual std::string describe() const override;
    virtual Person* clone() const override;
    virtual std::string typeTag() const override;
};

#endif
