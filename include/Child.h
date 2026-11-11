#ifndef RELIEF_CHILD_H
#define RELIEF_CHILD_H

#include "Resident.h"

// [Single inheritance] A camp resident aged 3-12. Children draw a reduced
// ration compared with adults.
// [Hierarchical inheritance] Infant, Child and Adult all derive from Resident.
class Child : public Resident {
public:
    Child();
    Child(const std::string& name, int age, char sex, int familyId, int campId);
    Child(const Child& other);
    ~Child() override;

    // [Overriding a pure virtual]
    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
