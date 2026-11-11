#include "TestRunner.h"
#include "Ration.h"
#include "RationNorms.h"
#include <sstream>

void run_ration_tests() {
    testing::beginSuite("Ration and RationNorms");

    // Default constructor gives an empty ration.
    Ration empty;
    ASSERT_TRUE(empty.isZero());
    ASSERT_NEAR(empty.get(Item::Rice), 0.0, 1e-9);
    ASSERT_NEAR(empty.totalWeight(), 0.0, 1e-9);

    // The norm table is the single source of truth.
    Ration adult = RationNorms::forAdult();
    ASSERT_NEAR(adult.get(Item::Rice), 0.400, 1e-9);
    ASSERT_NEAR(adult.get(Item::Dal), 0.060, 1e-9);
    ASSERT_NEAR(adult.get(Item::Oil), 0.030, 1e-9);
    ASSERT_NEAR(adult.get(Item::Salt), 0.010, 1e-9);
    ASSERT_NEAR(adult.get(Item::BabyFood), 0.0, 1e-9);
    ASSERT_EQ(RationNorms::tableCount(), 5);

    Ration infant = RationNorms::forInfant();
    ASSERT_NEAR(infant.get(Item::Rice), 0.0, 1e-9);        // infants get no rice
    ASSERT_NEAR(infant.get(Item::BabyFood), 0.100, 1e-9);
    ASSERT_TRUE(infant.get(Item::ORS) > 0.0);

    Ration child = RationNorms::forChild();
    ASSERT_NEAR(child.get(Item::Rice), 0.200, 1e-9);        // half the adult norm

    Ration elderly = RationNorms::forElderly();
    ASSERT_TRUE(elderly.get(Item::Rice) < adult.get(Item::Rice));
    ASSERT_TRUE(elderly.get(Item::ORS) > adult.get(Item::ORS));

    // [Basic type to class type] a headcount converts to an adult-equivalent ration.
    Ration forThree = 3;
    ASSERT_NEAR(forThree.get(Item::Rice), 1.200, 1e-9);
    ASSERT_NEAR(forThree.get(Item::Dal), 0.180, 1e-9);

    // Binary operators.
    Ration sum = adult + child;
    ASSERT_NEAR(sum.get(Item::Rice), 0.600, 1e-9);

    Ration diff = sum - child;
    ASSERT_NEAR(diff.get(Item::Rice), 0.400, 1e-9);
    ASSERT_TRUE(diff == adult);

    Ration doubled = adult * 2.0;
    ASSERT_NEAR(doubled.get(Item::Rice), 0.800, 1e-9);

    // Compound assignment.
    Ration acc;
    acc += adult;
    acc += adult;
    ASSERT_NEAR(acc.get(Item::Rice), 0.800, 1e-9);
    acc *= 0.5;
    ASSERT_NEAR(acc.get(Item::Rice), 0.400, 1e-9);

    // Subscript read access.
    ASSERT_NEAR(adult[Item::Rice], 0.400, 1e-9);

    // set / add
    Ration manual;
    manual.set(Item::Tarpaulin, 2.0);
    manual.add(Item::Tarpaulin, 1.0);
    ASSERT_NEAR(manual.get(Item::Tarpaulin), 3.0, 1e-9);
    ASSERT_FALSE(manual.isZero());

    // Copy constructor is a real copy.
    Ration copied(adult);
    copied.set(Item::Rice, 9.9);
    ASSERT_NEAR(adult.get(Item::Rice), 0.400, 1e-9);
    ASSERT_NEAR(copied.get(Item::Rice), 9.9, 1e-9);

    // Stream output names every non-zero item.
    std::ostringstream oss;
    oss << adult;
    std::string text = oss.str();
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("0.4") != std::string::npos);
    ASSERT_TRUE(text.find("Baby food") == std::string::npos);  // zero items are skipped

    // Bonuses exist and are small.
    ASSERT_TRUE(RationNorms::pregnancyRiceBonus() > 0.0);
    ASSERT_TRUE(RationNorms::lactationOilBonus() > 0.0);
}
