#include "TestRunner.h"
#include "DistributionQueue.h"
#include "FamilySorter.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include <vector>
#include <stdexcept>
#include <string>

// Builds a family with the given counts, so each test family has a predictable
// vulnerability score: infant 40, child 20, adult 10, elderly 35.
static Family* makeFamily(const std::string& head, int infants, int children,
                          int adults, int elderly, int adultAge) {
    Family* f = new Family(head, "Morigaon");
    for (int i = 0; i < infants;  i++) f->addMember(new Infant("Inf", 1, 'F', f->getId(), 0));
    for (int i = 0; i < children; i++) f->addMember(new Child("Chi", 8, 'M', f->getId(), 0));
    for (int i = 0; i < adults;   i++) f->addMember(new Adult("Adu", adultAge, 'M', f->getId(), 0));
    for (int i = 0; i < elderly;  i++) f->addMember(new Elderly("Eld", 70, 'F', f->getId(), 0));
    return f;
}

void run_queue_tests() {
    testing::beginSuite("DistributionQueue and FamilySorter");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    Family* low    = makeFamily("Jiten Saikia", 0, 0, 2, 0, 40);   // score 20
    Family* medium = makeFamily("Rupa Nath",    0, 2, 2, 0, 35);   // score 60
    Family* high   = makeFamily("Bipul Das",    1, 1, 2, 1, 30);   // score 115
    Family* middle = makeFamily("Gitali Pegu",  0, 1, 2, 1, 50);   // score 75

    ASSERT_EQ(low->vulnerabilityScore(), 20);
    ASSERT_EQ(medium->vulnerabilityScore(), 60);
    ASSERT_EQ(high->vulnerabilityScore(), 115);
    ASSERT_EQ(middle->vulnerabilityScore(), 75);

    // --- the hand-written heap ---
    DistributionQueue q;
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
    ASSERT_TRUE(q.peek() == 0);
    ASSERT_TRUE(q.pop() == 0);

    // Pushed in a deliberately jumbled order.
    q.push(medium);
    q.push(high);
    q.push(low);
    q.push(middle);
    ASSERT_EQ(q.size(), 4);
    ASSERT_FALSE(q.empty());

    // peek() does not remove.
    ASSERT_TRUE(q.peek() == high);
    ASSERT_EQ(q.size(), 4);

    // Most vulnerable first, every time.
    ASSERT_TRUE(q.pop() == high);     // 115
    ASSERT_TRUE(q.pop() == middle);   //  75
    ASSERT_TRUE(q.pop() == medium);   //  60
    ASSERT_TRUE(q.pop() == low);      //  20
    ASSERT_TRUE(q.empty());
    ASSERT_TRUE(q.pop() == 0);

    // A single element, and duplicates of equal priority, both behave.
    DistributionQueue one;
    one.push(low);
    ASSERT_TRUE(one.pop() == low);
    ASSERT_TRUE(one.empty());

    DistributionQueue ties;
    Family* tieA = makeFamily("Tie A", 0, 0, 1, 0, 30);   // score 10
    Family* tieB = makeFamily("Tie B", 0, 0, 1, 0, 31);   // score 10
    ties.push(tieA);
    ties.push(tieB);
    ASSERT_EQ(ties.size(), 2);
    Family* first = ties.pop();
    Family* second = ties.pop();
    ASSERT_TRUE(first != 0 && second != 0);
    ASSERT_TRUE(first != second);
    ASSERT_TRUE((first == tieA && second == tieB) || (first == tieB && second == tieA));
    ASSERT_TRUE(ties.empty());

    // A larger run, to shake out the sift logic rather than trusting four items.
    DistributionQueue many;
    std::vector<Family*> built;
    for (int i = 0; i < 25; i++) {
        // adults = (i % 5) + 1 gives scores of 10, 20, 30, 40, 50 in a cycle
        Family* f = makeFamily("Bulk " + std::to_string(i), 0, 0, (i % 5) + 1, 0, 30);
        built.push_back(f);
        many.push(f);
    }
    ASSERT_EQ(many.size(), 25);
    int previous = 1000000;
    int popped = 0;
    while (!many.empty()) {
        Family* f = many.pop();
        ASSERT_TRUE(f->vulnerabilityScore() <= previous);   // never increases
        previous = f->vulnerabilityScore();
        popped++;
    }
    ASSERT_EQ(popped, 25);

    // Scrambled input with a repeated score: pop everything and compare the
    // whole sequence. Adult counts 1..9 give scores 10..90.
    int scrambled[10] = {5, 2, 8, 1, 9, 4, 7, 3, 6, 2};
    int expectedOrder[10] = {9, 8, 7, 6, 5, 4, 3, 2, 2, 1};
    DistributionQueue scr;
    std::vector<Family*> scrFamilies;
    for (int i = 0; i < 10; i++) {
        Family* f = makeFamily("Scr " + std::to_string(i), 0, 0, scrambled[i], 0, 30);
        scrFamilies.push_back(f);
        scr.push(f);
    }
    for (int i = 0; i < 10; i++) {
        ASSERT_EQ(scr.peek()->vulnerabilityScore(), expectedOrder[i] * 10);
        Family* f = scr.pop();
        ASSERT_EQ(f->vulnerabilityScore(), expectedOrder[i] * 10);
    }
    ASSERT_TRUE(scr.empty());

    // Interleaved push and pop keeps the heap valid.
    scr.push(scrFamilies[3]);   // 10
    scr.push(scrFamilies[4]);   // 90
    ASSERT_TRUE(scr.pop() == scrFamilies[4]);
    scr.push(scrFamilies[2]);   // 80
    ASSERT_TRUE(scr.pop() == scrFamilies[2]);
    ASSERT_TRUE(scr.pop() == scrFamilies[3]);
    ASSERT_TRUE(scr.empty());

    // push(0) is ignored.
    scr.push(0);
    ASSERT_TRUE(scr.empty());

    q.push(low);
    q.push(high);
    q.clear();
    ASSERT_TRUE(q.empty());
    ASSERT_TRUE(q.pop() == 0);

    // --- FamilySorter: one pointer-to-member drives every ordering ---
    std::vector<Family*> list;
    list.push_back(low);
    list.push_back(high);
    list.push_back(medium);
    list.push_back(middle);

    FamilySorter::sortDescending(list, &Family::metricVulnerability);
    ASSERT_TRUE(list[0] == high);
    ASSERT_TRUE(list[3] == low);

    FamilySorter::sortAscending(list, &Family::metricVulnerability);
    ASSERT_TRUE(list[0] == low);
    ASSERT_TRUE(list[3] == high);

    FamilySorter::sortDescending(list, &Family::metricSize);
    ASSERT_EQ(list[0]->size(), 5);        // high has 5 members
    ASSERT_EQ(list[3]->size(), 2);        // low has 2

    FamilySorter::sortDescending(list, &Family::metricInfantCount);
    ASSERT_TRUE(list[0] == high);         // the only family with an infant

    // Averages: middle 44.5, low 40, high 27.8, medium 21.5. (The brief expected
    // low first, but middle's adults aged 50 make it the oldest household.)
    FamilySorter::sortDescending(list, &Family::metricAverageAge);
    ASSERT_TRUE(list[0] == middle);
    ASSERT_TRUE(list[1] == low);
    ASSERT_TRUE(list[2] == high);
    ASSERT_TRUE(list[3] == medium);

    // The menu looks a metric up by name, so the console never hard-codes four
    // separate sort calls.
    FamilySorter::Metric m = FamilySorter::metricByName("vulnerability");
    FamilySorter::sortDescending(list, m);
    ASSERT_TRUE(list[0] == high);

    ASSERT_TRUE(FamilySorter::metricByName("size") == &Family::metricSize);
    ASSERT_TRUE(FamilySorter::metricByName("age") == &Family::metricAverageAge);
    ASSERT_TRUE(FamilySorter::metricByName("infants") == &Family::metricInfantCount);
    ASSERT_THROWS(FamilySorter::metricByName("height"), std::invalid_argument);
    ASSERT_TRUE(FamilySorter::metricMenu().find("vulnerability") != std::string::npos);

    // Full descending order by size: high 5, then medium and middle (4 each), then low 2.
    FamilySorter::sortDescending(list, &Family::metricSize);
    ASSERT_TRUE(list[0] == high);
    ASSERT_TRUE(list[3] == low);
    ASSERT_EQ(list[1]->size(), 4);
    ASSERT_EQ(list[2]->size(), 4);

    // Sorting an empty or single-element list must not crash.
    std::vector<Family*> emptyList;
    FamilySorter::sortDescending(emptyList, &Family::metricSize);
    ASSERT_EQ(static_cast<int>(emptyList.size()), 0);
    FamilySorter::sortAscending(emptyList, &Family::metricSize);
    std::vector<Family*> singleList;
    singleList.push_back(low);
    FamilySorter::sortDescending(singleList, &Family::metricSize);
    ASSERT_EQ(static_cast<int>(singleList.size()), 1);
    ASSERT_TRUE(singleList[0] == low);

    for (int i = 0; i < static_cast<int>(built.size()); i++) delete built[i];
    for (int i = 0; i < static_cast<int>(scrFamilies.size()); i++) delete scrFamilies[i];
    delete low; delete medium; delete high; delete middle; delete tieA; delete tieB;

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
