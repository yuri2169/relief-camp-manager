#ifndef RELIEF_FAMILYSORTER_H
#define RELIEF_FAMILYSORTER_H

#include <vector>
#include <string>

class Family;

// Sorts families by whichever field the official picks from the menu.
//
// [Pointer to member function] Metric points at one of Family's metric
// functions. One sort routine therefore covers vulnerability, household size,
// average age and infant count - four near-identical functions collapsed into
// one, with the choice made at run time.
//
// The vectors hold non-owning Family* observers; sorting only reorders the
// pointers and never deletes a family.
class FamilySorter {
public:
    typedef double (Family::*Metric)() const;

    static void sortDescending(std::vector<Family*>& families, Metric metric);
    static void sortAscending(std::vector<Family*>& families, Metric metric);

    // Looks a metric up by the name shown in the menu.
    // Accepts "vulnerability", "size", "age", "infants".
    static Metric metricByName(const std::string& name);   // throws std::invalid_argument
    static std::string metricMenu();
};

#endif
