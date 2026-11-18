#include "DistributionLog.h"
#include "Exceptions.h"
#include <cstddef>
#include <stdexcept>

DistributionLog::DistributionLog() {}

bool DistributionLog::alreadyClaimed(int familyId, const Date& date) const {
    for (std::size_t i = 0; i < entries.size(); ++i) {
        // The rule: one claim per family per date, in any camp (camp is ignored).
        if (entries[i].getFamilyId() == familyId && entries[i].getDate() == date) return true;
    }
    return false;
}

void DistributionLog::record(const Allocation& a) {
    // Check first, so a refused claim leaves the log untouched.
    if (alreadyClaimed(a.getFamilyId(), a.getDate())) {
        throw DuplicateClaimException(a.getFamilyId(), a.getDate());
    }
    entries.push_back(a);
}

int DistributionLog::size() const { return static_cast<int>(entries.size()); }

const Allocation& DistributionLog::at(int index) const {
    if (index < 0 || index >= size()) {
        throw std::out_of_range("distribution index out of range");
    }
    return entries[static_cast<std::size_t>(index)];
}

std::vector<Allocation> DistributionLog::forCamp(int campId, const Date& date) const {
    std::vector<Allocation> result;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].getCampId() == campId && entries[i].getDate() == date) {
            result.push_back(entries[i]);
        }
    }
    return result;
}

std::vector<Allocation> DistributionLog::forFamily(int familyId) const {
    std::vector<Allocation> result;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].getFamilyId() == familyId) result.push_back(entries[i]);
    }
    return result;
}

Ration DistributionLog::totalForCamp(int campId, const Date& date) const {
    Ration total;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].getCampId() == campId && entries[i].getDate() == date) {
            total += entries[i].getAmount();
        }
    }
    return total;
}

int DistributionLog::familiesServed(int campId, const Date& date) const {
    // Count distinct family ids. Records loaded from a CSV may repeat a
    // family, so counting entries alone could over-count.
    std::vector<int> seen;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].getCampId() != campId || !(entries[i].getDate() == date)) continue;
        bool known = false;
        for (std::size_t j = 0; j < seen.size(); ++j) {
            if (seen[j] == entries[i].getFamilyId()) known = true;
        }
        if (!known) seen.push_back(entries[i].getFamilyId());
    }
    return static_cast<int>(seen.size());
}

void DistributionLog::loadEntry(const Allocation& a) { entries.push_back(a); }

void DistributionLog::clear() { entries.clear(); }
