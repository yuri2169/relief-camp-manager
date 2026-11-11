#include "TestRunner.h"
#include "HealthMonitor.h"
#include <stdexcept>
#include <vector>

void run_health_tests() {
    testing::beginSuite("HealthMonitor and the outbreak alert");

    // The thresholds are static, so they are the camp's norms, not one
    // monitor's settings.
    ASSERT_EQ(HealthMonitor::windowDays(), 3);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::Diarrhoea), 10);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::Fever), 15);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::SkinInfection), 20);

    HealthMonitor health;
    ASSERT_EQ(health.size(), 0);

    Date d12(2026, 7, 12);
    Date d13(2026, 7, 13);
    Date d14(2026, 7, 14);
    Date d15(2026, 7, 15);
    Date d11(2026, 7, 11);

    // Logging accumulates per camp, per day, per symptom.
    health.logCase(1, d12, Symptom::Diarrhoea, 3);
    health.logCase(1, d13, Symptom::Diarrhoea, 4);
    ASSERT_EQ(health.size(), 2);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 7);   // d12, d13, d14
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));   // 7 < 10

    // One more case crosses the threshold and the alert fires.
    health.logCase(1, d14, Symptom::Diarrhoea, 3);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);
    ASSERT_TRUE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));    // 10 >= 10

    // The window is exactly 3 days: [today-2, today]. A case from the 11th is
    // outside the window on the 14th, which is what keeps an old, resolved
    // cluster from raising a false alarm.
    health.logCase(1, d11, Symptom::Diarrhoea, 50);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);  // the 50 is ignored
    ASSERT_EQ(health.casesInWindow(1, d13, Symptom::Diarrhoea), 57);  // d11, d12, d13

    // Future cases are not counted either.
    health.logCase(1, d15, Symptom::Diarrhoea, 40);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);

    // Rolling forward, the oldest day drops out of the window.
    // On the 15th the window is d13, d14, d15 = 4 + 3 + 40 = 47.
    ASSERT_EQ(health.casesInWindow(1, d15, Symptom::Diarrhoea), 47);
    ASSERT_TRUE(health.outbreakAlert(1, d15, Symptom::Diarrhoea));

    // Symptoms are tracked separately.
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Fever), 0);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Fever));
    health.logCase(1, d14, Symptom::Fever, 14);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Fever));       // 14 < 15
    health.logCase(1, d14, Symptom::Fever, 1);
    ASSERT_TRUE(health.outbreakAlert(1, d14, Symptom::Fever));        // 15 >= 15

    // Camps are tracked separately: camp 2's silence is not camp 1's outbreak.
    ASSERT_EQ(health.casesInWindow(2, d14, Symptom::Diarrhoea), 0);
    ASSERT_FALSE(health.outbreakAlert(2, d14, Symptom::Diarrhoea));
    health.logCase(2, d14, Symptom::Diarrhoea, 12);
    ASSERT_TRUE(health.outbreakAlert(2, d14, Symptom::Diarrhoea));
    ASSERT_FALSE(health.outbreakAlert(2, d14, Symptom::Fever));

    // activeAlerts lists every symptom over its threshold at one camp.
    std::vector<Symptom> alerts = health.activeAlerts(1, d14);
    ASSERT_EQ(static_cast<int>(alerts.size()), 2);                    // diarrhoea + fever
    std::vector<Symptom> camp2Alerts = health.activeAlerts(2, d14);
    ASSERT_EQ(static_cast<int>(camp2Alerts.size()), 1);
    ASSERT_TRUE(camp2Alerts[0] == Symptom::Diarrhoea);
    ASSERT_EQ(static_cast<int>(health.activeAlerts(9, d14).size()), 0);

    // Queries and loading.
    ASSERT_TRUE(health.size() > 0);
    ASSERT_EQ(health.at(0).getCampId(), 1);
    ASSERT_TRUE(health.at(0).getSymptom() == Symptom::Diarrhoea);
    ASSERT_EQ(health.at(0).getCount(), 3);
    ASSERT_THROWS(health.at(999), std::out_of_range);
    ASSERT_TRUE(static_cast<int>(health.forCamp(1).size()) > 0);
    ASSERT_EQ(static_cast<int>(health.forCamp(9).size()), 0);

    HealthMonitor loaded;
    loaded.loadCase(HealthCase(1, d14, Symptom::Diarrhoea, 11));
    ASSERT_EQ(loaded.size(), 1);
    ASSERT_TRUE(loaded.outbreakAlert(1, d14, Symptom::Diarrhoea));

    // A zero or negative count is ignored rather than corrupting the total.
    HealthMonitor guarded;
    guarded.logCase(1, d14, Symptom::Fever, 0);
    guarded.logCase(1, d14, Symptom::Fever, -5);
    ASSERT_EQ(guarded.casesInWindow(1, d14, Symptom::Fever), 0);

    health.clear();
    ASSERT_EQ(health.size(), 0);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));

    // Window edges on their own, one camp, one symptom: today-2 is inside,
    // today-3 is outside, a future date is outside.
    HealthMonitor edge;
    edge.logCase(5, Date(2026, 7, 11), Symptom::Fever, 100);  // today-3: outside
    edge.logCase(5, Date(2026, 7, 12), Symptom::Fever, 1);    // today-2: inside
    edge.logCase(5, Date(2026, 7, 15), Symptom::Fever, 1000); // future: outside
    ASSERT_EQ(edge.casesInWindow(5, d14, Symptom::Fever), 1);
    // Month boundary: window for 1 Aug is 30 Jul .. 1 Aug.
    HealthMonitor wrap;
    wrap.logCase(1, Date(2026, 7, 30), Symptom::Fever, 4);
    wrap.logCase(1, Date(2026, 7, 29), Symptom::Fever, 99);
    ASSERT_EQ(wrap.casesInWindow(1, Date(2026, 8, 1), Symptom::Fever), 4);
    // One below and exactly at the threshold, with the case on today-2.
    HealthMonitor thr;
    thr.logCase(1, d12, Symptom::Diarrhoea, 9);
    ASSERT_FALSE(thr.outbreakAlert(1, d14, Symptom::Diarrhoea));
    thr.logCase(1, d14, Symptom::Diarrhoea, 1);
    ASSERT_TRUE(thr.outbreakAlert(1, d14, Symptom::Diarrhoea));
    ASSERT_FALSE(thr.outbreakAlert(1, d15, Symptom::Diarrhoea));  // d12 has aged out: 1 left
}
