#ifndef RELIEF_ALLOCATION_H
#define RELIEF_ALLOCATION_H

#include "Ration.h"
#include "Date.h"
#include <ostream>

// One handout: this family, at this camp, on this day, received this much.
// The unit of record for the distribution log.
class Allocation {
    int familyId;
    int campId;
    Date date;
    Ration amount;
public:
    Allocation();
    Allocation(int familyId, int campId, const Date& date, const Ration& amount);
    Allocation(const Allocation& other);
    ~Allocation();

    int getFamilyId() const;
    int getCampId() const;
    const Date& getDate() const;
    const Ration& getAmount() const;

    // [Class to class conversion] an Allocation can stand in for the Ration it
    // carries, which is what lets `stock - allocation` be written at all.
    operator Ration() const;

    // [Friend function] Two allocations are "the same claim" when the same
    // family claimed on the same day - the camp does NOT matter, because a
    // family must not be able to collect twice by walking to another camp.
    friend bool operator==(const Allocation& a, const Allocation& b);
    friend std::ostream& operator<<(std::ostream& os, const Allocation& a);
};

#endif
