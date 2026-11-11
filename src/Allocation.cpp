#include "Allocation.h"

Allocation::Allocation() : familyId(0), campId(0), date(), amount() {}

Allocation::Allocation(int familyId, int campId, const Date& date, const Ration& amount)
    : familyId(familyId), campId(campId), date(date), amount(amount) {}

Allocation::Allocation(const Allocation& other)
    : familyId(other.familyId), campId(other.campId),
      date(other.date), amount(other.amount) {}

Allocation::~Allocation() {}

int Allocation::getFamilyId() const { return familyId; }
int Allocation::getCampId() const { return campId; }
const Date& Allocation::getDate() const { return date; }
const Ration& Allocation::getAmount() const { return amount; }

Allocation::operator Ration() const { return amount; }

bool operator==(const Allocation& a, const Allocation& b) {
    return a.familyId == b.familyId && a.date == b.date;
}

std::ostream& operator<<(std::ostream& os, const Allocation& a) {
    return os << "family " << a.familyId << " at camp " << a.campId
              << " on " << a.date << ": " << a.amount;
}
