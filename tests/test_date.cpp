#include "TestRunner.h"
#include "Date.h"
#include "Item.h"
#include "Symptom.h"
#include <sstream>
#include <stdexcept>

void run_date_tests() {
    testing::beginSuite("Item, Symptom and Date");

    // --- Item ---
    ASSERT_EQ(ITEM_COUNT, 7);
    ASSERT_STR_EQ(itemName(Item::Rice), "Rice");
    ASSERT_STR_EQ(itemName(Item::BabyFood), "Baby food");
    ASSERT_STR_EQ(itemUnit(Item::Rice), "kg");
    ASSERT_STR_EQ(itemUnit(Item::Tarpaulin), "sheets");
    ASSERT_EQ(itemIndex(Item::Rice), 0);
    ASSERT_TRUE(itemAt(4) == Item::ORS);
    ASSERT_TRUE(itemFromName("Baby food") == Item::BabyFood);
    ASSERT_THROWS(itemFromName("Biscuits"), std::invalid_argument);

    // --- Symptom ---
    ASSERT_EQ(SYMPTOM_COUNT, 5);
    ASSERT_STR_EQ(symptomName(Symptom::Diarrhoea), "Diarrhoea");
    ASSERT_TRUE(symptomFromName("Fever") == Symptom::Fever);
    ASSERT_THROWS(symptomFromName("Hiccups"), std::invalid_argument);

    // --- Date construction and validity ---
    Date defaulted;                          // default constructor
    ASSERT_EQ(defaulted.getYear(), 1970);
    ASSERT_EQ(defaulted.getMonth(), 1);
    ASSERT_EQ(defaulted.getDay(), 1);

    Date d(2026, 7, 14);                     // parameterised constructor
    ASSERT_EQ(d.getYear(), 2026);
    ASSERT_EQ(d.getMonth(), 7);
    ASSERT_EQ(d.getDay(), 14);
    ASSERT_TRUE(d.isValid());
    ASSERT_STR_EQ(d.toString(), "2026-07-14");

    Date copy(d);                            // copy constructor
    ASSERT_TRUE(copy == d);

    ASSERT_FALSE(Date(2026, 13, 1).isValid());
    ASSERT_FALSE(Date(2026, 2, 30).isValid());
    ASSERT_TRUE(Date(2024, 2, 29).isValid());   // leap year
    ASSERT_FALSE(Date(2026, 2, 29).isValid());  // not a leap year
    ASSERT_FALSE(Date(1900, 2, 29).isValid());  // century non-leap

    // --- parse ---
    ASSERT_TRUE(Date::parse("2026-07-14") == d);
    ASSERT_THROWS(Date::parse("14/07/2026"), std::invalid_argument);
    ASSERT_THROWS(Date::parse("2026-02-30"), std::invalid_argument);

    // --- arithmetic ---
    ASSERT_EQ(Date(1970, 1, 1).toDayNumber(), 0L);
    ASSERT_EQ(Date(1970, 1, 2).toDayNumber(), 1L);
    ASSERT_TRUE(Date::fromDayNumber(0) == Date(1970, 1, 1));
    ASSERT_TRUE(d.plusDays(1) == Date(2026, 7, 15));
    ASSERT_TRUE(d.plusDays(-14) == Date(2026, 6, 30));
    ASSERT_TRUE(Date(2026, 7, 31).plusDays(1) == Date(2026, 8, 1));
    ASSERT_TRUE(Date(2026, 12, 31).plusDays(1) == Date(2027, 1, 1));
    ASSERT_EQ(Date(2026, 7, 14) - Date(2026, 7, 11), 3L);   // days between
    ASSERT_EQ(Date(2026, 7, 11) - Date(2026, 7, 14), -3L);

    // --- comparison ---
    ASSERT_TRUE(Date(2026, 7, 11) < Date(2026, 7, 14));
    ASSERT_FALSE(Date(2026, 7, 14) < Date(2026, 7, 11));
    ASSERT_TRUE(Date(2026, 7, 14) <= Date(2026, 7, 14));
    ASSERT_TRUE(Date(2026, 7, 14) != Date(2026, 7, 11));

    // --- stream output ---
    std::ostringstream oss;
    oss << d;
    ASSERT_STR_EQ(oss.str(), "2026-07-14");
}
