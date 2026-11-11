#include "TestRunner.h"
#include "Resident.h"
#include "Volunteer.h"
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
}
