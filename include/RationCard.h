#ifndef RELIEF_RATIONCARD_H
#define RELIEF_RATIONCARD_H

#include "Ration.h"
#include <string>
#include <ostream>

// The slip an official hands a family: who they are, who is in the household,
// what they are entitled to each day. A snapshot, not a live view - it is a
// different kind of thing from a Family, which is why the conversion in
// Family is a class-to-class conversion and not just a getter.
class RationCard {
    int familyId;
    std::string headName;
    int campId;
    int memberCount;
    int infants;
    int children;
    int adults;
    int elderly;
    Ration daily;
    int vulnerability;
public:
    RationCard();
    RationCard(int familyId, const std::string& headName, int campId,
               int memberCount, int infants, int children, int adults,
               int elderly, const Ration& daily, int vulnerability);
    RationCard(const RationCard& other);
    ~RationCard();

    int getFamilyId() const;
    const std::string& getHeadName() const;
    int getCampId() const;
    int getMemberCount() const;
    int getInfants() const;
    int getChildren() const;
    int getAdults() const;
    int getElderly() const;
    const Ration& getDaily() const;
    int getVulnerability() const;

    std::string render() const;    // the boxed card text shown in the demo

    friend std::ostream& operator<<(std::ostream& os, const RationCard& c);
};

#endif
