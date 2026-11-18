#include "RationCard.h"
#include "Item.h"
#include <sstream>
#include <iomanip>
#include <cmath>

RationCard::RationCard()
    : familyId(0), headName(""), campId(0), memberCount(0), infants(0),
      children(0), adults(0), elderly(0), daily(), vulnerability(0) {}

RationCard::RationCard(int familyId, const std::string& headName, int campId,
                       int memberCount, int infants, int children, int adults,
                       int elderly, const Ration& daily, int vulnerability)
    : familyId(familyId), headName(headName), campId(campId),
      memberCount(memberCount), infants(infants), children(children),
      adults(adults), elderly(elderly), daily(daily),
      vulnerability(vulnerability) {}

RationCard::RationCard(const RationCard& other)
    : familyId(other.familyId), headName(other.headName), campId(other.campId),
      memberCount(other.memberCount), infants(other.infants),
      children(other.children), adults(other.adults), elderly(other.elderly),
      daily(other.daily), vulnerability(other.vulnerability) {}

RationCard::~RationCard() {}

int RationCard::getFamilyId() const { return familyId; }
const std::string& RationCard::getHeadName() const { return headName; }
int RationCard::getCampId() const { return campId; }
int RationCard::getMemberCount() const { return memberCount; }
int RationCard::getInfants() const { return infants; }
int RationCard::getChildren() const { return children; }
int RationCard::getAdults() const { return adults; }
int RationCard::getElderly() const { return elderly; }
const Ration& RationCard::getDaily() const { return daily; }
int RationCard::getVulnerability() const { return vulnerability; }

std::string RationCard::render() const {
    const std::string line = "+------------------------------------------+\n";
    std::ostringstream out;
    out << line;
    out << "| RATION CARD\n";
    out << line;
    out << "| Family ID     : " << familyId << "\n";
    out << "| Head          : " << headName << "\n";
    if (campId == 0) {
        out << "| Camp          : not yet admitted\n";
    } else {
        out << "| Camp          : " << campId << "\n";
    }
    out << "| Members       : " << memberCount << " (infants " << infants
        << ", children " << children << ", adults " << adults
        << ", elderly " << elderly << ")\n";
    out << "| Daily ration  :\n";
    bool any = false;
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item it = itemAt(i);
        double amount = daily.get(it);
        if (std::fabs(amount) <= 1e-9) continue;
        out << "|   " << itemName(it) << " " << std::setprecision(3)
            << amount << " " << itemUnit(it) << "\n";
        any = true;
    }
    if (!any) out << "|   (none)\n";
    out << "| Vulnerability : " << vulnerability << "\n";
    out << line;
    return out.str();
}

std::ostream& operator<<(std::ostream& os, const RationCard& c) {
    return os << c.render();
}
