#include "RationNorms.h"
#include "Ration.h"

// Item order: {Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin}
const double RationNorms::adultDaily[ITEM_COUNT]     = {0.400, 0.060, 0.030, 0.010, 0.10, 0.000, 0.00};
const double RationNorms::childDaily[ITEM_COUNT]     = {0.200, 0.040, 0.020, 0.008, 0.15, 0.000, 0.00};
const double RationNorms::infantDaily[ITEM_COUNT]    = {0.000, 0.000, 0.010, 0.002, 0.25, 0.100, 0.00};
const double RationNorms::elderlyDaily[ITEM_COUNT]   = {0.300, 0.060, 0.035, 0.010, 0.20, 0.000, 0.00};
const double RationNorms::volunteerDaily[ITEM_COUNT] = {0.450, 0.070, 0.030, 0.010, 0.10, 0.000, 0.00};
const double RationNorms::pregnancyRice = 0.100;
const double RationNorms::lactationOil  = 0.010;

Ration RationNorms::forAdult()     { return Ration(adultDaily); }
Ration RationNorms::forChild()     { return Ration(childDaily); }
Ration RationNorms::forInfant()    { return Ration(infantDaily); }
Ration RationNorms::forElderly()   { return Ration(elderlyDaily); }
Ration RationNorms::forVolunteer() { return Ration(volunteerDaily); }
const double* RationNorms::adultTable() { return adultDaily; }
double RationNorms::pregnancyRiceBonus() { return pregnancyRice; }
double RationNorms::lactationOilBonus()  { return lactationOil; }
int RationNorms::tableCount() { return 5; }
