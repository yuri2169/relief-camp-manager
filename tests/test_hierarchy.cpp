#include "TestRunner.h"
#include "Resident.h"
#include "Volunteer.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include <sstream>

// A concrete Resident, needed because Resident itself is still abstract.
class TestResident : public Resident {
public:
    TestResident(const std::string& name, int age, char sex, int familyId, int campId)
        : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {}
    Ration rationNeed() const override { return Ration(1); }
    int vulnerabilityScore() const override { return 1 + flagBonus(); }
    std::string describe() const override { return "test resident"; }
    Person* clone() const override { return new TestResident(*this); }
    std::string typeTag() const override { return "TEST_RESIDENT"; }
};

void run_hierarchy_tests() {
    testing::beginSuite("Person hierarchy: virtual bases");

    Person::resetIdCounter(1);

    // Resident adds camp placement on top of Person.
    TestResident r("Nripen Kalita", 38, 'M', 101, 7);
    ASSERT_STR_EQ(r.getName(), "Nripen Kalita");      // inherited from Person
    ASSERT_EQ(r.getFamilyId(), 101);
    ASSERT_EQ(r.getCampId(), 7);
    ASSERT_STR_EQ(r.getShelterLabel(), "");

    r.assignTo(9, "Hall-B");
    ASSERT_EQ(r.getCampId(), 9);
    ASSERT_STR_EQ(r.getShelterLabel(), "Hall-B");

    r.setFamilyId(202);
    ASSERT_EQ(r.getFamilyId(), 202);

    // Volunteer is concrete and gets the worker meal norm.
    Volunteer v("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross");
    ASSERT_STR_EQ(v.getSkill(), "Medic");
    ASSERT_STR_EQ(v.getShift(), "Morning");
    ASSERT_STR_EQ(v.getOrganisation(), "Red Cross");
    ASSERT_STR_EQ(v.typeTag(), "VOLUNTEER");
    ASSERT_NEAR(v.rationNeed().get(Item::Rice), 0.450, 1e-9);

    // A volunteer is not vulnerable; that is the point of the field.
    ASSERT_EQ(v.vulnerabilityScore(), 0);
    ASSERT_TRUE(v.describe().find("Medic") != std::string::npos);

    v.reassign("Cook", "Night");
    ASSERT_STR_EQ(v.getSkill(), "Cook");
    ASSERT_STR_EQ(v.getShift(), "Night");

    // Polymorphism through the shared base.
    Person* pv = &v;
    ASSERT_NEAR(pv->rationNeed().get(Item::Rice), 0.450, 1e-9);
    Person* clonedV = pv->clone();
    ASSERT_STR_EQ(clonedV->typeTag(), "VOLUNTEER");
    delete clonedV;

    // Both branches derive from exactly one Person, which is what `virtual` buys.
    Person* pr = &r;
    ASSERT_TRUE(pr->getId() == r.getId());

    Person::resetIdCounter(1);

    testing::beginSuite("Person hierarchy: age bands");

    Person::resetIdCounter(1);

    Infant baby("Mitu Das", 1, 'F', 101, 1);
    ASSERT_STR_EQ(baby.typeTag(), "INFANT");
    ASSERT_NEAR(baby.rationNeed().get(Item::Rice), 0.0, 1e-9);
    ASSERT_NEAR(baby.rationNeed().get(Item::BabyFood), 0.100, 1e-9);
    ASSERT_EQ(baby.vulnerabilityScore(), 40);
    ASSERT_TRUE(baby.describe().find("infant") != std::string::npos);

    Child kid("Rahul Das", 9, 'M', 101, 1);
    ASSERT_STR_EQ(kid.typeTag(), "CHILD");
    ASSERT_NEAR(kid.rationNeed().get(Item::Rice), 0.200, 1e-9);
    ASSERT_EQ(kid.vulnerabilityScore(), 20);

    Adult man("Bipul Das", 35, 'M', 101, 1);
    ASSERT_STR_EQ(man.typeTag(), "ADULT");
    ASSERT_NEAR(man.rationNeed().get(Item::Rice), 0.400, 1e-9);
    ASSERT_EQ(man.vulnerabilityScore(), 10);

    // Pregnancy raises both the ration and the score.
    Adult mother("Rina Das", 29, 'F', 101, 1);
    mother.setFlags(true, false, false);
    ASSERT_NEAR(mother.rationNeed().get(Item::Rice), 0.500, 1e-9);   // 0.400 + 0.100 bonus
    ASSERT_EQ(mother.vulnerabilityScore(), 35);                      // 10 + 25

    // Lactation raises the oil allowance.
    Adult nursing("Jonali Das", 24, 'F', 101, 1);
    nursing.setFlags(false, false, true);
    ASSERT_NEAR(nursing.rationNeed().get(Item::Oil), 0.040, 1e-9);   // 0.030 + 0.010
    ASSERT_EQ(nursing.vulnerabilityScore(), 25);                     // 10 + 15

    // All three answer through the same base pointer: runtime polymorphism.
    Person* people[3] = { &baby, &kid, &man };
    double totalRice = 0.0;
    int totalScore = 0;
    for (int i = 0; i < 3; i++) {
        totalRice += people[i]->rationNeed().get(Item::Rice);
        totalScore += people[i]->vulnerabilityScore();
    }
    ASSERT_NEAR(totalRice, 0.600, 1e-9);
    ASSERT_EQ(totalScore, 70);

    // Each clone keeps its own dynamic type, and the data of the original.
    for (int i = 0; i < 3; i++) {
        Person* c = people[i]->clone();
        ASSERT_STR_EQ(c->typeTag(), people[i]->typeTag());
        ASSERT_STR_EQ(c->getName(), people[i]->getName());
        ASSERT_STR_EQ(c->describe(), people[i]->describe());
        ASSERT_TRUE(c->getId() != people[i]->getId());
        delete c;
    }

    // They are all Residents, so camp placement works on every one of them.
    baby.assignTo(2, "Tent-14");
    ASSERT_EQ(baby.getCampId(), 2);

    Person::resetIdCounter(1);
}
