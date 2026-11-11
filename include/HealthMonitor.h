#ifndef RELIEF_HEALTHMONITOR_H
#define RELIEF_HEALTHMONITOR_H

#include "Symptom.h"
#include "Date.h"
#include <vector>
#include <ostream>

// One line in the camp's health register: this many cases of this symptom at
// this camp on this day.
class HealthCase {
    int campId;
    Date date;
    Symptom symptom;
    int count;
public:
    HealthCase();
    HealthCase(int campId, const Date& date, Symptom symptom, int count);
    int getCampId() const;
    const Date& getDate() const;
    Symptom getSymptom() const;
    int getCount() const;
    friend std::ostream& operator<<(std::ostream& os, const HealthCase& h);
};

// Watches the health register for the beginnings of an outbreak. Diarrhoea is
// the one that kills in a flood camp, because the water is contaminated, so it
// has the lowest threshold.
//
// The alert looks at a rolling 3-day window rather than a single day: one bad
// afternoon is not an outbreak, and three quiet days in a row should clear the
// alarm by themselves.
class HealthMonitor {
    // [Static data members] these are the camp's norms, not one monitor's settings
    static const int kWindowDays;
    static const int kDiarrhoeaThreshold;
    static const int kFeverThreshold;
    static const int kDefaultThreshold;

    std::vector<HealthCase> cases;

public:
    HealthMonitor();

    void logCase(int campId, const Date& date, Symptom symptom, int count);

    // Total cases at one camp, of one symptom, in [today - 2, today].
    int casesInWindow(int campId, const Date& today, Symptom symptom) const;
    bool outbreakAlert(int campId, const Date& today, Symptom symptom) const;
    std::vector<Symptom> activeAlerts(int campId, const Date& today) const;

    // [Static member functions]
    static int thresholdFor(Symptom symptom);
    static int windowDays();

    int size() const;
    const HealthCase& at(int index) const;          // throws std::out_of_range
    std::vector<HealthCase> forCamp(int campId) const;
    void loadCase(const HealthCase& c);             // CSV loading
    void clear();
};

#endif
