#ifndef RELIEF_INFANT_H
#define RELIEF_INFANT_H

#include "Resident.h"

// A camp resident aged 0-2. Infants need baby food and
// ORS rather than rice, and they are the most vulnerable group in a flood camp.
// Infant, Child and Adult all derive from Resident.
class Infant : public Resident {
public:
    Infant();
    Infant(const std::string& name, int age, char sex, int familyId, int campId);
    Infant(const Infant& other);
    ~Infant() override;

    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
