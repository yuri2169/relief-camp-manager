#include "HealthMonitor.h"
#include <stdexcept>

// ---------------------------------------------------------------- HealthCase

HealthCase::HealthCase() : campId(0), date(), symptom(Symptom::Other), count(0) {}

HealthCase::HealthCase(int campId, const Date& date, Symptom symptom, int count)
    : campId(campId), date(date), symptom(symptom), count(count) {}

int HealthCase::getCampId() const { return campId; }
const Date& HealthCase::getDate() const { return date; }
Symptom HealthCase::getSymptom() const { return symptom; }
int HealthCase::getCount() const { return count; }

std::ostream& operator<<(std::ostream& os, const HealthCase& h) {
    os << "camp " << h.campId << " " << h.date << " " << symptomName(h.symptom)
       << " x" << h.count;
    return os;
}

// ------------------------------------------------------------- HealthMonitor

// The camp's norms: a 3-day window, and the case counts that raise the alarm.
const int HealthMonitor::kWindowDays = 3;
const int HealthMonitor::kDiarrhoeaThreshold = 10;
const int HealthMonitor::kFeverThreshold = 15;
const int HealthMonitor::kDefaultThreshold = 20;

HealthMonitor::HealthMonitor() {}

void HealthMonitor::logCase(int campId, const Date& date, Symptom symptom, int count) {
    if (count <= 0) return;   // a zero or negative count must not corrupt the total
    cases.push_back(HealthCase(campId, date, symptom, count));
}

int HealthMonitor::casesInWindow(int campId, const Date& today, Symptom symptom) const {
    int total = 0;
    for (const HealthCase& c : cases) {
        if (c.getCampId() != campId || c.getSymptom() != symptom) continue;
        // Window is [today - 2, today]: age 0, 1 or 2 days. A future-dated
        // case has a negative age and is not counted toward today's window.
        // [Operator overloading in use] Date::operator- gives the age in days
        long age = today - c.getDate();
        if (age >= 0 && age < kWindowDays) total += c.getCount();
    }
    return total;
}

bool HealthMonitor::outbreakAlert(int campId, const Date& today, Symptom symptom) const {
    // At the threshold is already an outbreak (>=, not >).
    return casesInWindow(campId, today, symptom) >= thresholdFor(symptom);
}

std::vector<Symptom> HealthMonitor::activeAlerts(int campId, const Date& today) const {
    std::vector<Symptom> result;
    for (int i = 0; i < SYMPTOM_COUNT; ++i) {
        Symptom s = static_cast<Symptom>(i);
        if (outbreakAlert(campId, today, s)) result.push_back(s);
    }
    return result;
}

int HealthMonitor::thresholdFor(Symptom symptom) {
    switch (symptom) {
        case Symptom::Diarrhoea: return kDiarrhoeaThreshold;
        case Symptom::Fever:     return kFeverThreshold;
        default:                 return kDefaultThreshold;
    }
}

int HealthMonitor::windowDays() { return kWindowDays; }

int HealthMonitor::size() const { return static_cast<int>(cases.size()); }

const HealthCase& HealthMonitor::at(int index) const {
    if (index < 0 || index >= size()) throw std::out_of_range("health case index out of range");
    return cases[index];
}

std::vector<HealthCase> HealthMonitor::forCamp(int campId) const {
    std::vector<HealthCase> result;
    for (const HealthCase& c : cases) {
        if (c.getCampId() == campId) result.push_back(c);
    }
    return result;
}

void HealthMonitor::loadCase(const HealthCase& c) { cases.push_back(c); }

void HealthMonitor::clear() { cases.clear(); }
