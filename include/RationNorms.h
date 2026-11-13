#ifndef RELIEF_RATIONNORMS_H
#define RELIEF_RATIONNORMS_H

#include "Item.h"

class Ration;   // forward declaration keeps the include graph acyclic

// The relief ration norms, in one place. Every quantity in the system traces
// back to this table.
class RationNorms {
    // Shared by every object, defined once in the .cpp
    static const double adultDaily[ITEM_COUNT];
    static const double childDaily[ITEM_COUNT];
    static const double infantDaily[ITEM_COUNT];
    static const double elderlyDaily[ITEM_COUNT];
    static const double volunteerDaily[ITEM_COUNT];
    static const double pregnancyRice;
    static const double lactationOil;
public:
    // No object needed to read a norm
    static Ration forAdult();
    static Ration forChild();
    static Ration forInfant();
    static Ration forElderly();
    static Ration forVolunteer();
    static const double* adultTable();
    static double pregnancyRiceBonus();
    static double lactationOilBonus();
    static int tableCount();
};

#endif
