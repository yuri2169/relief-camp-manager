#ifndef RELIEF_TRANSFERADVISOR_H
#define RELIEF_TRANSFERADVISOR_H

#include "Item.h"
#include <string>
#include <vector>
#include <ostream>

class Camp;

// "Move 220 kg of rice from camp 3 to camp 1." A value object, built and
// returned by TransferAdvisor::suggest.
class TransferSuggestion {
    int fromCampId;
    int toCampId;
    std::string fromCampName;
    std::string toCampName;
    Item item;
    double quantity;
    double fromDays;        // donor's days of supply before the move
    double toDays;          // receiver's days of supply before the move
public:
    TransferSuggestion();
    TransferSuggestion(int fromCampId, const std::string& fromCampName,
                       int toCampId, const std::string& toCampName,
                       Item item, double quantity, double fromDays, double toDays);
    int getFromCampId() const;
    int getToCampId() const;
    const std::string& getFromCampName() const;
    const std::string& getToCampName() const;
    Item getItem() const;
    double getQuantity() const;
    double getFromDays() const;
    double getToDays() const;
    std::string render() const;
    friend std::ostream& operator<<(std::ostream& os, const TransferSuggestion& t);
};

// Looks across the camps and proposes moves: an item one camp is about to run
// out of, taken from a camp that has plenty. Nothing is moved automatically -
// a human approves the transfer, because trucks and boats are involved.
//
// The camps are passed as plain Camp* observers. The advisor never owns,
// deletes or stores them: they belong to the Registry<Camp> (or wherever the
// caller keeps them), and the pointers are only used during the call. A
// pointer taken from a Registry stops being valid once that registry removes
// or clears the camp, so do not keep the vector around.
class TransferAdvisor {
    double criticalDays;    // at or below this, a camp is in trouble
    double surplusDays;     // above this, a camp can afford to give
public:
    TransferAdvisor();                                      // 3.0 and 7.0
    TransferAdvisor(double criticalDays, double surplusDays);
    double getCriticalDays() const;
    double getSurplusDays() const;

    std::vector<Item> criticalItems(const Camp& camp) const;

    // [Objects as arguments] [Returning objects]
    std::vector<TransferSuggestion> suggest(const std::vector<Camp*>& camps) const;
};

#endif
