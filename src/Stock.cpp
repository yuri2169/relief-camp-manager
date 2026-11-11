#include "Stock.h"
#include "Allocation.h"
#include "Exceptions.h"
#include <cmath>
#include <iomanip>
#include <sstream>

const double Stock::kUnlimitedDays = 999.0;

// Anything at or below this counts as "no demand" / "nothing missing".
static const double kEpsilon = 1e-9;

Stock::Stock() : dailyDemand() {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = 0.0;
}

Stock::Stock(const Stock& other) : dailyDemand(other.dailyDemand) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = other.qty[i];
}

Stock::~Stock() {}

double Stock::get(Item it) const { return qty[itemIndex(it)]; }
void Stock::set(Item it, double value) { qty[itemIndex(it)] = value; }
void Stock::add(Item it, double value) { qty[itemIndex(it)] += value; }

void Stock::take(Item it, double value) {
    int i = itemIndex(it);
    // Check first, change second: a refused take leaves the stock untouched.
    if (value > qty[i] + kEpsilon) {
        throw InsufficientStockException(it, value, qty[i]);
    }
    qty[i] -= value;
}

void Stock::setDailyDemand(const Ration& demand) { dailyDemand = demand; }
const Ration& Stock::getDailyDemand() const { return dailyDemand; }

double Stock::daysOfSupplyFor(Item it) const {
    double perDay = dailyDemand.get(it);
    if (perDay <= kEpsilon) return kUnlimitedDays;
    return qty[itemIndex(it)] / perDay;
}

double Stock::daysOfSupply() const {
    double worst = kUnlimitedDays;
    for (int i = 0; i < ITEM_COUNT; i++) {
        double d = daysOfSupplyFor(itemAt(i));
        if (d < worst) worst = d;
    }
    return worst;
}

Item Stock::mostCriticalItem() const {
    int worstIndex = 0;
    double worst = daysOfSupplyFor(itemAt(0));
    for (int i = 1; i < ITEM_COUNT; i++) {
        double d = daysOfSupplyFor(itemAt(i));
        if (d < worst) {            // strict, so ties keep the lower index
            worst = d;
            worstIndex = i;
        }
    }
    return itemAt(worstIndex);
}

Stock::operator double() const { return daysOfSupply(); }

double Stock::operator[](Item it) const { return get(it); }

Stock& Stock::operator+=(const Stock& other) {
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] += other.qty[i];
    return *this;
}

Stock& Stock::operator-=(const Allocation& a) {
    const Ration& amount = a.getAmount();
    double result[ITEM_COUNT];
    // Work out the whole result first so a refusal changes nothing.
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item it = itemAt(i);
        double wanted = amount.get(it);
        if (wanted > qty[i] + kEpsilon) {
            throw InsufficientStockException(it, wanted, qty[i]);
        }
        result[i] = qty[i] - wanted;
        if (result[i] < 0.0) result[i] = 0.0;   // absorb the tolerance
    }
    for (int i = 0; i < ITEM_COUNT; i++) qty[i] = result[i];
    return *this;
}

Stock operator+(const Stock& a, const Stock& b) {
    Stock result(a);
    result += b;
    return result;
}

Stock operator-(const Stock& s, const Allocation& a) {
    Stock result(s);
    result -= a;
    return result;
}

std::ostream& operator<<(std::ostream& os, const Stock& s) {
    // Format into a local stream so the caller's stream flags stay untouched.
    std::ostringstream text;
    text << std::setprecision(4);
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item it = itemAt(i);
        if (i > 0) text << ", ";
        text << itemName(it) << " " << s.qty[i] << " " << itemUnit(it);
    }
    if (!s.dailyDemand.isZero()) {
        text << " (worst: " << itemName(s.mostCriticalItem()) << ", "
             << s.daysOfSupply() << " days)";
    }
    return os << text.str();
}
