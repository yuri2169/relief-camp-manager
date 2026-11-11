#ifndef RELIEF_RATION_H
#define RELIEF_RATION_H

#include "Item.h"
#include <ostream>

// One day's entitlement, item by item. The unit of each slot is itemUnit().
class Ration {
    double qty[ITEM_COUNT];
public:
    Ration();                                  // all zero
    // [Basic type to class type] non-explicit on purpose
    Ration(int persons);
    explicit Ration(const double* table);      // build from a RationNorms table
    // [Copy constructor]
    Ration(const Ration& other);
    ~Ration();

    // [Const member function]
    double get(Item it) const;
    void set(Item it, double value);
    void add(Item it, double value);
    double totalWeight() const;
    bool isZero() const;

    // [Binary operator] read access
    double operator[](Item it) const;
    Ration& operator+=(const Ration& other);
    Ration& operator*=(double factor);

    // [Friend functions] symmetric arithmetic reads better as free functions
    friend Ration operator+(const Ration& a, const Ration& b);
    friend Ration operator-(const Ration& a, const Ration& b);
    friend Ration operator*(const Ration& r, double factor);
    friend bool operator==(const Ration& a, const Ration& b);
    friend std::ostream& operator<<(std::ostream& os, const Ration& r);
};

#endif
