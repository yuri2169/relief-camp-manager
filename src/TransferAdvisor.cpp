#include "TransferAdvisor.h"
#include "Camp.h"
#include <iomanip>
#include <sstream>

// Anything at or below this counts as "no demand" / "nothing missing".
static const double kEpsilon = 1e-9;

// Does this camp actually consume the item? Stock::daysOfSupplyFor() answers
// kUnlimitedDays (999) both for "no demand at all" and, via daysOfSupply(), as
// a cap, so 999 alone never means "huge surplus". A camp with no demand for an
// item may simply be empty or unregistered, so it is neither short of the item
// nor able to spare it. Every classification below checks this first.
static bool hasDemandFor(const Camp& camp, Item item) {
    return camp.getStock().getDailyDemand().get(item) > kEpsilon;
}

// ------------------------------------------------------- TransferSuggestion

TransferSuggestion::TransferSuggestion()
    : fromCampId(0), toCampId(0), fromCampName(), toCampName(),
      item(Item::Rice), quantity(0.0), fromDays(0.0), toDays(0.0) {}

TransferSuggestion::TransferSuggestion(int fromCampId, const std::string& fromCampName,
                                       int toCampId, const std::string& toCampName,
                                       Item item, double quantity,
                                       double fromDays, double toDays)
    : fromCampId(fromCampId), toCampId(toCampId),
      fromCampName(fromCampName), toCampName(toCampName),
      item(item), quantity(quantity), fromDays(fromDays), toDays(toDays) {}

int TransferSuggestion::getFromCampId() const { return fromCampId; }
int TransferSuggestion::getToCampId() const { return toCampId; }
const std::string& TransferSuggestion::getFromCampName() const { return fromCampName; }
const std::string& TransferSuggestion::getToCampName() const { return toCampName; }
Item TransferSuggestion::getItem() const { return item; }
double TransferSuggestion::getQuantity() const { return quantity; }
double TransferSuggestion::getFromDays() const { return fromDays; }
double TransferSuggestion::getToDays() const { return toDays; }

std::string TransferSuggestion::render() const {
    // Format into a local stream; quantity keeps up to 10 significant digits
    // so 60 prints as "60" and 62.5 as "62.5", days always show one decimal.
    std::ostringstream text;
    text << "Move " << std::setprecision(10) << quantity << " "
         << itemUnit(item) << " of " << itemName(item)
         << " from " << fromCampName << " (#" << fromCampId << ", "
         << std::fixed << std::setprecision(1) << fromDays << " days)"
         << " to " << toCampName << " (#" << toCampId << ", "
         << toDays << " days)";
    return text.str();
}

std::ostream& operator<<(std::ostream& os, const TransferSuggestion& t) {
    return os << t.render();
}

// ---------------------------------------------------------- TransferAdvisor

TransferAdvisor::TransferAdvisor() : criticalDays(3.0), surplusDays(7.0) {}

TransferAdvisor::TransferAdvisor(double criticalDays, double surplusDays)
    : criticalDays(criticalDays), surplusDays(surplusDays) {}

double TransferAdvisor::getCriticalDays() const { return criticalDays; }
double TransferAdvisor::getSurplusDays() const { return surplusDays; }

std::vector<Item> TransferAdvisor::criticalItems(const Camp& camp) const {
    std::vector<Item> result;
    for (int i = 0; i < ITEM_COUNT; i++) {
        Item it = itemAt(i);
        if (!hasDemandFor(camp, it)) continue;      // no demand, so no shortage
        if (camp.getStock().daysOfSupplyFor(it) <= criticalDays) result.push_back(it);
    }
    return result;
}

// [Objects as arguments] [Returning objects]
std::vector<TransferSuggestion> TransferAdvisor::suggest(const std::vector<Camp*>& camps) const {
    std::vector<TransferSuggestion> result;

    for (size_t t = 0; t < camps.size(); t++) {
        const Camp* to = camps[t];
        for (int i = 0; i < ITEM_COUNT; i++) {
            Item item = itemAt(i);
            double toDays = to->getStock().daysOfSupplyFor(item);
            if (toDays > criticalDays) continue;
            if (!hasDemandFor(*to, item)) continue;     // no demand, so no shortage
            double toDemand = to->getStock().getDailyDemand().get(item);
            double needed = criticalDays * toDemand - to->getStock().get(item);
            if (needed <= kEpsilon) continue;

            // Pick the donor with the most days of supply.
            const Camp* best = nullptr;
            double bestDays = 0.0;
            double bestSpare = 0.0;
            for (size_t f = 0; f < camps.size(); f++) {
                const Camp* from = camps[f];
                if (from == to) continue;
                // A donor must really consume the item: 999 days for a camp
                // with no demand means "unknown", not "plenty to spare".
                if (!hasDemandFor(*from, item)) continue;
                double fromDays = from->getStock().daysOfSupplyFor(item);
                if (fromDays <= surplusDays) continue;
                double fromDemand = from->getStock().getDailyDemand().get(item);
                double spare = from->getStock().get(item) - surplusDays * fromDemand;
                if (spare <= kEpsilon) continue;
                if (fromDays > bestDays) {
                    best = from;
                    bestDays = fromDays;
                    bestSpare = spare;
                }
            }

            if (best != nullptr) {
                double quantity = needed < bestSpare ? needed : bestSpare;
                result.push_back(TransferSuggestion(best->getId(), best->getName(),
                                                    to->getId(), to->getName(),
                                                    item, quantity, bestDays, toDays));
            }
        }
    }
    return result;
}
