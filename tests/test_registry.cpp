#include "TestRunner.h"
#include "Registry.h"
#include "Algorithms.h"
#include "Family.h"
#include "Adult.h"
#include "Infant.h"
#include "Volunteer.h"
#include <memory>

void run_registry_tests() {
    testing::beginSuite("Registry<T> and findMax");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    Registry<Family> families;
    ASSERT_EQ(families.size(), 0);
    ASSERT_TRUE(families.findById(1) == 0);
    ASSERT_TRUE(families.at(0) == 0);

    // add() takes ownership and hands back a usable pointer.
    Family* a = families.add(std::unique_ptr<Family>(new Family("Bipul Das", "Bhuragaon")));
    Family* b = families.add(std::unique_ptr<Family>(new Family("Jiten Saikia", "Mayong")));
    Family* c = families.add(std::unique_ptr<Family>(new Family("Rupa Nath", "Laharighat")));
    ASSERT_EQ(families.size(), 3);
    ASSERT_STR_EQ(a->getHeadName(), "Bipul Das");

    // findById works through the template's requirement that T has getId().
    ASSERT_TRUE(families.findById(a->getId()) == a);
    ASSERT_TRUE(families.findById(b->getId()) == b);
    ASSERT_TRUE(families.findById(99999) == 0);

    // at() is positional.
    ASSERT_TRUE(families.at(0) == a);
    ASSERT_TRUE(families.at(2) == c);
    ASSERT_TRUE(families.at(3) == 0);
    ASSERT_TRUE(families.at(-1) == 0);

    // all() gives a flat view for iteration.
    std::vector<Family*> view = families.all();
    ASSERT_EQ(static_cast<int>(view.size()), 3);
    ASSERT_TRUE(view[1] == b);

    // remove() frees the element; the registry shrinks.
    int bId = b->getId();
    ASSERT_TRUE(families.remove(bId));
    ASSERT_EQ(families.size(), 2);
    ASSERT_TRUE(families.findById(bId) == 0);
    ASSERT_FALSE(families.remove(bId));        // already gone

    // The same template over a different type, proving it is generic.
    Registry<Volunteer> helpers;
    Volunteer* vol = helpers.add(std::unique_ptr<Volunteer>(
        new Volunteer("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross")));
    ASSERT_EQ(helpers.size(), 1);
    ASSERT_TRUE(helpers.findById(vol->getId()) == vol);
    ASSERT_STR_EQ(helpers.at(0)->getSkill(), "Medic");

    // --- findMax over a pointer-to-member metric ---
    a->addMember(new Adult("Bipul Das", 35, 'M', a->getId(), 0));          // score 10
    c->addMember(new Infant("Mitu Nath", 1, 'F', c->getId(), 0));          // score 40
    c->addMember(new Adult("Rupa Nath", 30, 'F', c->getId(), 0));          // score 10

    std::vector<Family*> all = families.all();
    Family* mostVulnerable = findMax(all, &Family::metricVulnerability);
    ASSERT_TRUE(mostVulnerable == c);
    ASSERT_NEAR(mostVulnerable->metricVulnerability(), 50.0, 1e-9);

    Family* biggest = findMax(all, &Family::metricSize);
    ASSERT_TRUE(biggest == c);

    Family* oldest = findMax(all, &Family::metricAverageAge);
    ASSERT_TRUE(oldest == a);                   // average 35 beats (1+30)/2

    std::vector<Family*> none;
    ASSERT_TRUE(findMax(none, &Family::metricSize) == 0);

    // clear() empties the registry and frees everything.
    int liveBefore = Person::liveObjects();
    ASSERT_TRUE(liveBefore > 0);
    families.clear();
    ASSERT_EQ(families.size(), 0);
    // The families took their members with them. Only the one Volunteer still
    // held by `helpers` is left alive, so the count dropped to exactly 1.
    ASSERT_EQ(Person::liveObjects(), 1);
    helpers.clear();
    ASSERT_EQ(Person::liveObjects(), 0);        // now the unique_ptr-owned Volunteer is freed too

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
