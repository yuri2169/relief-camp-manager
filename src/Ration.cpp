#include "Ration.h"
#include "RationNorms.h"
#include <cmath>
#include <iomanip>
#include <sstream>

Ration::Ration() {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = 0.0;
}

Ration::Ration(int persons) {
    if (persons < 0) persons = 0;
    const double* adult = RationNorms::adultTable();
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = adult[i] * persons;
}

Ration::Ration(const double* table) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = table[i];
}

Ration::Ration(const Ration& other) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = other.qty[i];
}

Ration::~Ration() {}

double Ration::get(Item it) const { return qty[itemIndex(it)]; }
void Ration::set(Item it, double value) { qty[itemIndex(it)] = value; }
void Ration::add(Item it, double value) { qty[itemIndex(it)] += value; }

double Ration::totalWeight() const {
    double total = 0.0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        std::string unit = itemUnit(itemAt(i));
        if (unit == "kg" || unit == "litre") total += qty[i];
    }
    return total;
}

bool Ration::isZero() const {
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (std::fabs(qty[i]) > 1e-9) return false;
    }
    return true;
}

double Ration::operator[](Item it) const { return get(it); }

Ration& Ration::operator+=(const Ration& other) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] += other.qty[i];
    return *this;
}

Ration& Ration::operator*=(double factor) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] *= factor;
    return *this;
}

Ration operator+(const Ration& a, const Ration& b) {
    Ration result(a);
    result += b;
    return result;
}

// A ration can never go negative, so each slot is clamped at zero.
Ration operator-(const Ration& a, const Ration& b) {
    Ration result;
    for (int i = 0; i < ITEM_COUNT; i++) {
        double d = a.qty[i] - b.qty[i];
        result.qty[i] = (d > 0.0) ? d : 0.0;
    }
    return result;
}

Ration operator*(const Ration& r, double factor) {
    Ration result(r);
    result *= factor;
    return result;
}

bool operator==(const Ration& a, const Ration& b) {
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (std::fabs(a.qty[i] - b.qty[i]) > 1e-9) return false;
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const Ration& r) {
    // Format into a local stream so the caller's stream flags stay untouched.
    std::ostringstream text;
    text << std::setprecision(3);
    bool first = true;
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (std::fabs(r.qty[i]) <= 1e-9) continue;
        if (!first) text << ", ";
        Item it = itemAt(i);
        text << itemName(it) << " " << r.qty[i] << " " << itemUnit(it);
        first = false;
    }
    return os << text.str();
}
