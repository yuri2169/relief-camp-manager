#include "TestRunner.h"
#include "Person.h"
#include <sstream>

// A minimal concrete Person defined inside the test, so this task can be
// verified before any of the real subclasses exist.
class TestPerson : public Person {
public:
    TestPerson() : Person() {}
    TestPerson(const std::string& name, int age, char sex) : Person(name, age, sex) {}
    TestPerson(const TestPerson& other) : Person(other) {}
    Ration rationNeed() const override { Ration r; r.set(Item::Rice, 1.0); return r; }
    int vulnerabilityScore() const override { return 7 + flagBonus(); }
    std::string describe() const override { return "test person " + getName(); }
    Person* clone() const override { return new TestPerson(*this); }
    std::string typeTag() const override { return "TEST"; }
};

// Abstractness check (compile-time, verified by hand and not left in the tree):
// adding `Person bad;` to this file fails with "variable type 'Person' is an
// abstract class".
void run_person_tests() {
    testing::beginSuite("Person abstract base");

    Person::resetIdCounter(1);

    // Parameterised constructor and const getters.
    TestPerson a("Rupa Das", 34, 'F');
    ASSERT_STR_EQ(a.getName(), "Rupa Das");
    ASSERT_EQ(a.getAge(), 34);
    ASSERT_EQ(a.getSex(), 'F');
    ASSERT_EQ(a.getId(), 1);
    ASSERT_FALSE(a.isPregnant());
    ASSERT_FALSE(a.isDisabled());
    ASSERT_FALSE(a.isLactating());

    // Every new Person takes the next ID.
    TestPerson b("Bhaskar Nath", 41, 'M');
    ASSERT_EQ(b.getId(), 2);
    ASSERT_EQ(Person::lastIssuedId(), 2);

    // Default constructor still takes an ID.
    TestPerson blank;
    ASSERT_EQ(blank.getId(), 3);
    ASSERT_STR_EQ(blank.getName(), "unnamed");
    ASSERT_EQ(blank.getAge(), 0);

    // Copy constructor copies the fields but takes a fresh identity.
    TestPerson copied(a);
    ASSERT_STR_EQ(copied.getName(), "Rupa Das");
    ASSERT_EQ(copied.getAge(), 34);
    ASSERT_TRUE(copied.getId() != a.getId());

    // Flags feed the shared vulnerability bonus.
    TestPerson mother("Anjali Bora", 27, 'F');
    ASSERT_EQ(mother.vulnerabilityScore(), 7);
    mother.setFlags(true, false, false);        // pregnant
    ASSERT_TRUE(mother.isPregnant());
    ASSERT_EQ(mother.vulnerabilityScore(), 7 + 25);
    mother.setFlags(false, true, true);         // disabled and lactating
    ASSERT_EQ(mother.vulnerabilityScore(), 7 + 20 + 15);

    // Runtime polymorphism through a base pointer.
    Person* p = &a;
    ASSERT_NEAR(p->rationNeed().get(Item::Rice), 1.0, 1e-9);
    ASSERT_STR_EQ(p->describe(), "test person Rupa Das");
    ASSERT_STR_EQ(p->typeTag(), "TEST");

    // clone() produces an independent object of the same dynamic type.
    Person* cl = p->clone();
    ASSERT_STR_EQ(cl->getName(), "Rupa Das");
    ASSERT_STR_EQ(cl->typeTag(), "TEST");
    ASSERT_TRUE(cl != p);
    delete cl;

    // adoptId lets CSV loading restore the original identity.
    TestPerson loaded("Jiten Saikia", 55, 'M');
    loaded.adoptId(4321);
    ASSERT_EQ(loaded.getId(), 4321);

    // resetIdCounter keeps freshly created objects from colliding after a load.
    Person::resetIdCounter(5000);
    TestPerson afterLoad("Minoti Devi", 60, 'F');
    ASSERT_EQ(afterLoad.getId(), 5000);

    // Friend operator<< delegates to describe().
    std::ostringstream oss;
    oss << a;
    ASSERT_TRUE(oss.str().find("Rupa Das") != std::string::npos);
    ASSERT_TRUE(oss.str().find("#1") != std::string::npos);

    Person::resetIdCounter(1);
}
