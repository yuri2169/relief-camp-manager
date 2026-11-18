#ifndef RELIEF_DISTRIBUTIONQUEUE_H
#define RELIEF_DISTRIBUTIONQUEUE_H

#include <vector>

class Family;

// A priority queue over families, most vulnerable first, so an infant's family
// is served before a household of healthy adults when a queue forms.
//
// This is a binary max-heap we wrote ourselves rather than std::priority_queue,
// so the data structure is visible and our overloaded Family::operator< is
// plainly what drives the ordering.
//
// The queue does NOT own the families; it only borrows pointers to them.
// It never deletes a Family, and the caller must keep each family alive while
// it is in the queue.
class DistributionQueue {
    std::vector<Family*> heap;

    void siftUp(int index);
    void siftDown(int index);
    // Uses Family::operator< so the priority rule lives in one place.
    bool higherPriority(const Family* a, const Family* b) const;

public:
    DistributionQueue();

    void push(Family* f);
    Family* pop();                 // 0 when empty
    const Family* peek() const;    // 0 when empty
    bool empty() const;
    int size() const;
    void clear();
};

#endif
