#ifndef RELIEF_ELDERLY_H
#define RELIEF_ELDERLY_H

#include "Adult.h"

// A resident aged 60 or over: less rice, more ORS and oil, and a much higher
// vulnerability score, because the elderly are prioritised in a flood camp.
// Elderly -> Adult -> Resident -> Person, four levels.
class Elderly : public Adult {
public:
    Elderly();
    Elderly(const std::string& name, int age, char sex, int familyId, int campId);
    Elderly(const Elderly& other);
    ~Elderly() override;

    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
