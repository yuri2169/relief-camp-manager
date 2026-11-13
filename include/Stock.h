#ifndef RELIEF_STOCK_H
#define RELIEF_STOCK_H

#include "Item.h"
#include "Ration.h"
#include <ostream>

class Allocation;   // only the .cpp needs the full definition

// What a camp physically holds, plus the camp's current daily draw. Keeping the
// draw here is what lets a Stock answer "how many days do we have left?" on its
// own, and therefore what makes the conversion to double meaningful.
class Stock {
    double qty[ITEM_COUNT];
    Ration dailyDemand;
public:
    // [Static data member] the answer when nothing is being consumed
    static const double kUnlimitedDays;

    Stock();
    Stock(const Stock& other);
    ~Stock();

    double get(Item it) const;
    void set(Item it, double value);
    void add(Item it, double value);
    void take(Item it, double value);          // throws InsufficientStockException

    void setDailyDemand(const Ration& demand);
    const Ration& getDailyDemand() const;

    double daysOfSupplyFor(Item it) const;
    double daysOfSupply() const;               // the worst item decides
    Item mostCriticalItem() const;

    // [Class to basic type conversion] `double d = stock;` gives the days of
    // supply. A daysOfSupply(demand) method would be a cleaner design; the
    // demand is stored above so that the no-argument conversion can work.
    operator double() const;

    double operator[](Item it) const;
    Stock& operator+=(const Stock& other);
    Stock& operator-=(const Allocation& a);    // [Object as argument]

    // [Friend binary operators]
    friend Stock operator+(const Stock& a, const Stock& b);
    friend Stock operator-(const Stock& s, const Allocation& a);
    friend std::ostream& operator<<(std::ostream& os, const Stock& s);
};

#endif
