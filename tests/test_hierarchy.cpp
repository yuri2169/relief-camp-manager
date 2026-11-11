#include "TestRunner.h"
#include "Resident.h"
#include "Volunteer.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
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

    testing::beginSuite("Person hierarchy: multilevel and the diamond");

    Person::resetIdCounter(1);

    // [Multilevel inheritance] Elderly -> Adult -> Resident -> Person
    Elderly grandma("Minoti Devi", 72, 'F', 101, 1);
    ASSERT_STR_EQ(grandma.typeTag(), "ELDERLY");
    ASSERT_NEAR(grandma.rationNeed().get(Item::Rice), 0.300, 1e-9);
    ASSERT_TRUE(grandma.rationNeed().get(Item::ORS) > 0.100);
    ASSERT_EQ(grandma.vulnerabilityScore(), 35);
    ASSERT_TRUE(grandma.describe().find("elderly") != std::string::npos);

    ASSERT_EQ(grandma.getFamilyId(), 101);
    ASSERT_STR_EQ(grandma.getName(), "Minoti Devi");
    Adult* asAdult = &grandma;
    ASSERT_NEAR(asAdult->rationNeed().get(Item::Rice), 0.300, 1e-9);

    Elderly frail("Hemanta Bora", 80, 'M', 102, 1);
    frail.setFlags(false, true, false);
    ASSERT_EQ(frail.vulnerabilityScore(), 55);

    // The copy constructor names its bases, so name and family carry over.
    Elderly grandmaCopy(grandma);
    ASSERT_STR_EQ(grandmaCopy.getName(), "Minoti Devi");
    ASSERT_EQ(grandmaCopy.getFamilyId(), 101);

    // [Multiple inheritance] [Hybrid inheritance] a resident who also volunteers
    ResidentVolunteer helper("Gautam Pegu", 31, 'M', 103, 1, "Record keeper", "Evening");
    ASSERT_STR_EQ(helper.typeTag(), "RESIDENT_VOLUNTEER");
    ASSERT_EQ(helper.getFamilyId(), 103);
    ASSERT_STR_EQ(helper.getSkill(), "Record keeper");
    ASSERT_STR_EQ(helper.getShift(), "Evening");

    // [Virtual base class] exactly ONE Person subobject. Reaching Person
    // through each parent must give the same object, the same ID, the same name.
    Resident* viaResident = &helper;
    Volunteer* viaVolunteer = &helper;
    Person* viaResidentPerson = viaResident;
    Person* viaVolunteerPerson = viaVolunteer;
    ASSERT_TRUE(viaResidentPerson == viaVolunteerPerson);
    ASSERT_EQ(viaResident->getId(), viaVolunteer->getId());
    ASSERT_STR_EQ(viaResident->getName(), viaVolunteer->getName());
    ASSERT_STR_EQ(viaResident->getName(), "Gautam Pegu");
    ASSERT_EQ(helper.getId(), viaVolunteer->getId());
    ASSERT_EQ(viaResident->getAge(), 31);
    ASSERT_EQ(viaVolunteer->getAge(), 31);

    // One flag set too: setting a flag through one path is seen through the other.
    viaResident->setFlags(false, true, false);
    ASSERT_TRUE(viaVolunteer->isDisabled());
    ASSERT_EQ(helper.vulnerabilityScore(), 30);   // 10 + 20
    viaResident->setFlags(false, false, false);

    // describe() resolves through the Person interface without ambiguity.
    ASSERT_TRUE(viaResidentPerson->describe().find("resident volunteer") != std::string::npos);
    ASSERT_TRUE(viaVolunteerPerson->describe().find("Gautam Pegu") != std::string::npos);

    // Draws the family ration, not the worker meal.
    ASSERT_NEAR(helper.rationNeed().get(Item::Rice), 0.400, 1e-9);
    ASSERT_EQ(helper.vulnerabilityScore(), 10);
    ASSERT_TRUE(helper.describe().find("resident volunteer") != std::string::npos);

    // Cloning keeps the dynamic type across the diamond (the clone gets a new ID).
    Person* clonedHelper = viaVolunteerPerson->clone();
    ASSERT_STR_EQ(clonedHelper->typeTag(), "RESIDENT_VOLUNTEER");
    ASSERT_STR_EQ(clonedHelper->getName(), "Gautam Pegu");
    ASSERT_NEAR(clonedHelper->rationNeed().get(Item::Rice), 0.400, 1e-9);
    ASSERT_TRUE(clonedHelper->getId() != helper.getId());
    delete clonedHelper;

    Infant i2("Pori Pegu", 2, 'F', 103, 1);
    Child c2("Dipu Pegu", 7, 'M', 103, 1);
    Adult a2("Gitali Pegu", 30, 'F', 103, 1);
    Volunteer v2("Nurse Kaberi", 40, 'F', "Nurse", "Night", "District Health");
    Person* roster[6] = { &i2, &c2, &a2, &grandma, &helper, &v2 };
    std::string tags;
    for (int i = 0; i < 6; i++) { tags += roster[i]->typeTag(); tags += ";"; }
    ASSERT_STR_EQ(tags, "INFANT;CHILD;ADULT;ELDERLY;RESIDENT_VOLUNTEER;VOLUNTEER;");

    Person::resetIdCounter(1);
}
