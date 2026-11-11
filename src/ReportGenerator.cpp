#include "ReportGenerator.h"
#include "Camp.h"
#include "Family.h"
#include "DistributionLog.h"
#include "HealthMonitor.h"
#include "Exceptions.h"
#include <cstddef>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace {

// An item with this many days of supply or fewer is flagged LOW.
const double kLowDays = 3.0;

// Wraps a CSV value in quotes when it contains a comma or a quote, so every
// row keeps exactly three fields.
std::string csvField(const std::string& value) {
    if (value.find_first_of(",\"\n\r") == std::string::npos) {
        return value;
    }
    std::string quoted = "\"";
    for (std::size_t i = 0; i < value.size(); i++) {
        if (value[i] == '"') { quoted += '"'; }
        quoted += value[i];
    }
    quoted += "\"";
    return quoted;
}

// Stock::daysOfSupplyFor returns 999 only when the item has no daily demand;
// it never caps a real figure. (The aggregate daysOfSupply() can pin at 999
// when every demanded item exceeds it, but this report does not print it.)
// So "no demand" is decided by asking the demand itself, and any item that has
// demand is printed with its real computed number.
const double kEpsilon = 1e-9;

bool hasDemand(const Stock& stock, Item item) {
    return stock.getDailyDemand().get(item) > kEpsilon;
}

}  // namespace

ReportGenerator::ReportGenerator() {}

std::string ReportGenerator::stockTable(const Camp& camp) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    // [Friend class in use] camp.stock is a PRIVATE member of Camp.
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item item = itemAt(i);
        double days = camp.stock.daysOfSupplyFor(item);
        out << "  " << std::left << std::setw(10) << itemName(item)
            << std::right << std::setw(8) << camp.stock.get(item) << " "
            << std::left << std::setw(8) << itemUnit(item) << std::right;
        if (!hasDemand(camp.stock, item)) {
            out << "no daily demand";
        } else {
            out << std::setw(7) << days << " days";
            if (days <= kLowDays) { out << "  LOW"; }
        }
        out << "\n";
    }
    // The worst item is the one with demand and the fewest days; ties keep
    // the lower index. Items with no demand never qualify. Stock's
    // mostCriticalItem() is not used because, on a tie at 999 (no demand), it
    // returns the lowest index even when that item has no demand at all.
    bool found = false;
    Item worstItem = Item::Rice;
    double worst = 0.0;
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item item = itemAt(i);
        if (!hasDemand(camp.stock, item)) { continue; }
        double days = camp.stock.daysOfSupplyFor(item);
        if (!found || days < worst) {
            found = true;
            worstItem = item;
            worst = days;
        }
    }
    if (!found) {
        out << "Worst item: none (no daily demand recorded)\n";
    } else {
        out << "Worst item: " << itemName(worstItem) << " (" << worst << " days)\n";
    }
    return out.str();
}

std::string ReportGenerator::dailyReport(const Camp& camp, const Date& date,
                                         const std::vector<Family*>& families,
                                         const DistributionLog& log,
                                         const HealthMonitor& health) const {
    // Note for callers: "of N" counts the families passed in, while "Served
    // today" is this camp's log entries, so pass this camp's families.
    std::ostringstream out;
    out << "=====================================================\n";
    out << " DAILY CAMP REPORT   " << date << "\n";
    out << "=====================================================\n";
    // [Friend class in use] camp.id, camp.name, camp.district, camp.occupancy,
    // camp.capacity and camp.shelters are PRIVATE members of Camp. Only a
    // friend can read them.
    out << " Camp      : #" << camp.id << "  " << camp.name << "\n";
    out << " District  : " << camp.district << "\n";
    out << " Occupancy : " << camp.occupancy << " / " << camp.capacity;
    if (camp.isOverflowing()) { out << "   *** OVER CAPACITY ***"; }
    out << "\n\n";

    out << "SHELTERS\n";
    if (camp.shelters.empty()) { out << "  No shelters\n"; }
    for (std::size_t i = 0; i < camp.shelters.size(); i++) {
        const Camp::Shelter& s = camp.shelters[i];
        out << "  " << s.getLabel() << ": " << s.getOccupancy() << " / "
            << s.getCapacity() << "\n";
    }
    out << "\n";

    out << "STOCK\n" << stockTable(camp) << "\n";

    int campId = camp.id;
    out << "DISTRIBUTION\n";
    out << "Served today: " << log.familiesServed(campId, date) << " of "
        << static_cast<int>(families.size()) << " families\n";
    Ration issued = log.totalForCamp(campId, date);
    out << std::fixed << std::setprecision(2);
    if (issued.isZero()) { out << "  Nothing issued today\n"; }
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item item = itemAt(i);
        if (issued.get(item) > 0.0) {
            out << "  " << itemName(item) << ": " << issued.get(item) << " "
                << itemUnit(item) << "\n";
        }
    }
    out << "\n";

    out << "HEALTH\n";
    bool anyCases = false;
    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        Symptom symptom = static_cast<Symptom>(i);
        int cases = health.casesInWindow(campId, date, symptom);
        if (cases > 0) {
            out << "  " << symptomName(symptom) << ": " << cases
                << " cases in the last " << HealthMonitor::windowDays() << " days\n";
            anyCases = true;
        }
    }
    if (!anyCases) {
        out << "  No cases in the last " << HealthMonitor::windowDays() << " days\n";
    }
    std::vector<Symptom> alerts = health.activeAlerts(campId, date);
    for (std::size_t i = 0; i < alerts.size(); i++) {
        out << "  *** OUTBREAK ALERT: " << symptomName(alerts[i]) << " ***\n";
    }
    out << "\n";

    out << "FAMILIES\n";
    if (families.empty()) { out << "  No families\n"; }
    for (std::size_t i = 0; i < families.size(); i++) {
        const Family* f = families[i];
        if (f == nullptr) { continue; }
        out << "  #" << f->getId() << "  " << f->getHeadName() << "  ("
            << f->size() << " people, vulnerability " << f->vulnerabilityScore()
            << ")  " << (log.alreadyClaimed(f->getId(), date) ? "collected" : "PENDING")
            << "\n";
    }
    return out.str();
}

void ReportGenerator::exportCsv(const std::string& path, const Camp& camp,
                                const Date& date,
                                const std::vector<Family*>& families,
                                const DistributionLog& log,
                                const HealthMonitor& health) const {
    std::ofstream out(path.c_str());
    if (!out.is_open()) {
        throw FileFormatException(path, 0, "cannot open file for writing");
    }
    int campId = camp.id;
    out << "section,key,value\n";
    out << "report,date," << date << "\n";
    out << "camp,id," << campId << "\n";
    out << "camp,name," << csvField(camp.name) << "\n";
    out << "camp,district," << csvField(camp.district) << "\n";
    out << "camp,occupancy," << camp.occupancy << "\n";
    out << "camp,capacity," << camp.capacity << "\n";
    for (std::size_t i = 0; i < camp.shelters.size(); i++) {
        const Camp::Shelter& s = camp.shelters[i];
        out << "shelter," << csvField(s.getLabel()) << ","
            << s.getOccupancy() << "/" << s.getCapacity() << "\n";
    }
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item item = itemAt(i);
        out << "stock," << itemName(item) << "," << camp.stock.get(item) << "\n";
    }
    out << "distribution,families_served," << log.familiesServed(campId, date) << "\n";
    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        Symptom symptom = static_cast<Symptom>(i);
        int cases = health.casesInWindow(campId, date, symptom);
        if (cases > 0) {
            out << "health," << symptomName(symptom) << "," << cases << "\n";
        }
    }
    for (std::size_t i = 0; i < families.size(); i++) {
        const Family* f = families[i];
        if (f == nullptr) { continue; }
        out << "family," << csvField(f->getHeadName()) << ","
            << (log.alreadyClaimed(f->getId(), date) ? "collected" : "pending") << "\n";
    }
}
