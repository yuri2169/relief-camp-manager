#include "TestRunner.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "RationCard.h"
#include <sstream>
#include <stdexcept>

// Builds the demo family: one infant, one child, two adults, one elderly.
static Family makeDasFamily() {
    Family f("Bipul Das", "Bhuragaon");
    f.addMember(new Adult("Bipul Das", 35, 'M', f.getId(), 0));
    f.addMember(new Adult("Rina Das", 29, 'F', f.getId(), 0));
    f.addMember(new Child("Rahul Das", 9, 'M', f.getId(), 0));
    f.addMember(new Infant("Mitu Das", 1, 'F', f.getId(), 0));
    f.addMember(new Elderly("Minoti Devi", 72, 'F', f.getId(), 0));
    return f;
}

void run_family_tests() {
    testing::beginSuite("Family and the rule of three");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    // Default constructor.
    Family blank;
    ASSERT_EQ(blank.size(), 0);
    ASSERT_STR_EQ(blank.getHeadName(), "unnamed");
    ASSERT_EQ(blank.getId(), 1);

    // Parameterised constructor and growth past the initial capacity.
    Family f("Bipul Das", "Bhuragaon");
    ASSERT_STR_EQ(f.getHeadName(), "Bipul Das");
    ASSERT_STR_EQ(f.getVillage(), "Bhuragaon");
    ASSERT_EQ(f.size(), 0);

    for (int i = 0; i < 9; i++) {
        f.addMember(new Adult("Member " + std::to_string(i), 20 + i, 'M', f.getId(), 0));
    }
    ASSERT_EQ(f.size(), 9);                       // grew past the initial capacity of 4
    ASSERT_STR_EQ(f.memberAt(0).getName(), "Member 0");
    ASSERT_STR_EQ(f.memberAt(8).getName(), "Member 8");
    ASSERT_THROWS(f.memberAt(9), std::out_of_range);
    ASSERT_THROWS(f.memberAt(-1), std::out_of_range);

    // Polymorphic ration: each member answers with its own rationNeed().
    Family das = makeDasFamily();
    ASSERT_EQ(das.size(), 5);
    // 0.400 + 0.400 (adults) + 0.200 (child) + 0.000 (infant) + 0.300 (elderly)
    ASSERT_NEAR(das.dailyRation().get(Item::Rice), 1.300, 1e-9);
    ASSERT_NEAR(das.dailyRation().get(Item::BabyFood), 0.100, 1e-9);
    // 10 + 10 + 20 + 40 + 35
    ASSERT_EQ(das.vulnerabilityScore(), 115);

    ASSERT_EQ(das.countOfTag("INFANT"), 1);
    ASSERT_EQ(das.countOfTag("ADULT"), 2);
    ASSERT_EQ(das.countOfTag("ELDERLY"), 1);
    ASSERT_EQ(das.countOfTag("VOLUNTEER"), 0);

    // Metric functions, all double and all const, for pointer-to-member sorting.
    ASSERT_NEAR(das.metricVulnerability(), 115.0, 1e-9);
    ASSERT_NEAR(das.metricSize(), 5.0, 1e-9);
    ASSERT_NEAR(das.metricAverageAge(), (35 + 29 + 9 + 1 + 72) / 5.0, 1e-9);
    ASSERT_NEAR(das.metricInfantCount(), 1.0, 1e-9);
    ASSERT_NEAR(Family("Nobody", "Nowhere").metricAverageAge(), 0.0, 1e-9);  // no divide by zero

    // --- RULE OF THREE ---

    // Copy constructor deep-copies every member.
    Family copy(das);
    ASSERT_EQ(copy.size(), 5);
    ASSERT_NEAR(copy.dailyRation().get(Item::Rice), 1.300, 1e-9);
    // Different objects, and different Person IDs, because clone() makes new people.
    ASSERT_TRUE(&copy.memberAt(0) != &das.memberAt(0));
    ASSERT_TRUE(copy.memberAt(0).getId() != das.memberAt(0).getId());
    // The clone kept its dynamic type.
    ASSERT_STR_EQ(copy.memberAt(3).typeTag(), "INFANT");
    // Mutating the copy does not touch the original.
    copy.memberAt(0).setName("Changed");
    ASSERT_STR_EQ(das.memberAt(0).getName(), "Bipul Das");

    // Copy assignment deep-copies too, and frees what was there before.
    Family target("Temp Head", "Temp Village");
    target.addMember(new Adult("Throwaway", 50, 'M', target.getId(), 0));
    ASSERT_EQ(target.size(), 1);
    target = das;
    ASSERT_EQ(target.size(), 5);
    ASSERT_STR_EQ(target.getHeadName(), "Bipul Das");
    ASSERT_TRUE(&target.memberAt(2) != &das.memberAt(2));

    // Self-assignment must not corrupt or free the object.
    Family self = makeDasFamily();
    Family* selfPtr = &self;
    self = *selfPtr;
    ASSERT_EQ(self.size(), 5);
    ASSERT_NEAR(self.dailyRation().get(Item::Rice), 1.300, 1e-9);

    // Assignment returns a reference, so chaining works.
    Family c1, c2;
    c1 = c2 = das;
    ASSERT_EQ(c1.size(), 5);
    ASSERT_EQ(c2.size(), 5);

    // No leaks: every Person created must be destroyed. The live count returns
    // to its starting value once the scoped families go out of scope.
    int before = Person::liveObjects();
    {
        Family scoped = makeDasFamily();
        ASSERT_EQ(Person::liveObjects(), before + 5);
        Family scopedCopy(scoped);
        ASSERT_EQ(Person::liveObjects(), before + 10);
    }
    ASSERT_EQ(Person::liveObjects(), before);

    // --- OPERATORS ---

    Family lowRisk("Jiten Saikia", "Mayong");
    lowRisk.addMember(new Adult("Jiten Saikia", 40, 'M', lowRisk.getId(), 0));
    ASSERT_EQ(lowRisk.vulnerabilityScore(), 10);

    // operator< orders by vulnerability score, which the priority queue uses.
    ASSERT_TRUE(lowRisk < das);
    ASSERT_FALSE(das < lowRisk);

    // operator== is identity, for the duplicate-claim check.
    Family same = das;            // copy keeps the family ID
    ASSERT_TRUE(same == das);
    ASSERT_FALSE(lowRisk == das);

    // adoptId restores a loaded identity.
    Family loaded("Loaded Head", "Loaded Village");
    loaded.adoptId(7700);
    ASSERT_EQ(loaded.getId(), 7700);

    // operator<< summarises the family.
    std::ostringstream oss;
    oss << das;
    ASSERT_TRUE(oss.str().find("Bipul Das") != std::string::npos);
    ASSERT_TRUE(oss.str().find("5") != std::string::npos);

    // Extra memory proofs: assignment frees the old members, and self-assignment
    // neither leaks nor destroys anything.
    // (earlier families are still alive, so take a fresh baseline here)
    int baseline = Person::liveObjects();
    {
        int start = baseline;
        Family a = makeDasFamily();           // +5
        Family b("Other", "Elsewhere");
        b.addMember(new Adult("Old", 50, 'M', b.getId(), 0));   // +1
        ASSERT_EQ(Person::liveObjects(), start + 6);
        b = a;                                // old member freed, 5 cloned
        ASSERT_EQ(Person::liveObjects(), start + 10);
        Family* aPtr = &a;
        a = *aPtr;                            // self-assignment: nothing changes
        ASSERT_EQ(Person::liveObjects(), start + 10);
        ASSERT_EQ(a.size(), 5);
    }
    ASSERT_EQ(Person::liveObjects(), baseline);

    // Mutating a copy's membership leaves the original alone.
    {
        Family original = makeDasFamily();
        Family clone(original);
        clone.addMember(new Adult("Extra", 30, 'F', clone.getId(), 0));
        ASSERT_EQ(clone.size(), 6);
        ASSERT_EQ(original.size(), 5);
    }
    ASSERT_EQ(Person::liveObjects(), baseline);

    // addMember(nullptr) is ignored.
    {
        Family f2("X", "Y");
        f2.addMember(nullptr);
        ASSERT_EQ(f2.size(), 0);
    }

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    testing::beginSuite("Family to RationCard conversion");

    Person::resetIdCounter(1);
    Family::resetIdCounter(500);

    Family card_family = makeDasFamily();
    card_family.setCampId(3);

    // Implicit, on assignment to a RationCard.
    RationCard card = card_family;
    ASSERT_EQ(card.getFamilyId(), card_family.getId());
    ASSERT_STR_EQ(card.getHeadName(), "Bipul Das");
    ASSERT_EQ(card.getCampId(), 3);
    ASSERT_EQ(card.getMemberCount(), 5);
    ASSERT_EQ(card.getInfants(), 1);
    ASSERT_EQ(card.getChildren(), 1);
    ASSERT_EQ(card.getAdults(), 2);
    ASSERT_EQ(card.getElderly(), 1);
    ASSERT_EQ(card.getVulnerability(), 115);
    ASSERT_NEAR(card.getDaily().get(Item::Rice), 1.300, 1e-9);

    // The same conversion works explicitly, which is how the menu uses it.
    RationCard explicitCard = static_cast<RationCard>(card_family);
    ASSERT_EQ(explicitCard.getMemberCount(), 5);

    // Default constructor gives an empty card.
    RationCard blankCard;
    ASSERT_EQ(blankCard.getFamilyId(), 0);
    ASSERT_EQ(blankCard.getMemberCount(), 0);

    // render() produces the demo-facing card text.
    std::string text = card.render();
    ASSERT_TRUE(text.find("RATION CARD") != std::string::npos);
    ASSERT_TRUE(text.find("Bipul Das") != std::string::npos);
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("Vulnerability : 115") != std::string::npos);
    ASSERT_TRUE(text.find("Rice 1.3 kg") != std::string::npos);

    std::ostringstream cardOss;
    cardOss << card;
    ASSERT_TRUE(cardOss.str().find("Bipul Das") != std::string::npos);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
