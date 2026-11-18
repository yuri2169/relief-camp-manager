#include "DistributionQueue.h"
#include "Family.h"

DistributionQueue::DistributionQueue() {}

// `*b < *a` means "a outranks b", because
// Family::operator< compares vulnerability scores and we want the HIGHER
// score at the top of the heap.
bool DistributionQueue::higherPriority(const Family* a, const Family* b) const {
    return *b < *a;
}

void DistributionQueue::siftUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (!higherPriority(heap[index], heap[parent])) {
            break;
        }
        Family* temp = heap[index];
        heap[index] = heap[parent];
        heap[parent] = temp;
        index = parent;
    }
}

void DistributionQueue::siftDown(int index) {
    int count = static_cast<int>(heap.size());
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int best = index;
        if (left < count && higherPriority(heap[left], heap[best])) {
            best = left;
        }
        if (right < count && higherPriority(heap[right], heap[best])) {
            best = right;
        }
        if (best == index) {
            break;
        }
        Family* temp = heap[index];
        heap[index] = heap[best];
        heap[best] = temp;
        index = best;
    }
}

void DistributionQueue::push(Family* f) {
    if (f == 0) { return; }
    heap.push_back(f);
    siftUp(static_cast<int>(heap.size()) - 1);
}

Family* DistributionQueue::pop() {
    if (heap.empty()) { return 0; }
    Family* top = heap[0];
    heap[0] = heap[heap.size() - 1];
    heap.pop_back();
    if (!heap.empty()) { siftDown(0); }
    return top;
}

const Family* DistributionQueue::peek() const {
    return heap.empty() ? 0 : heap[0];
}

bool DistributionQueue::empty() const { return heap.empty(); }
int DistributionQueue::size() const { return static_cast<int>(heap.size()); }
void DistributionQueue::clear() { heap.clear(); }
