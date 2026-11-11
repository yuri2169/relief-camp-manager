#ifndef RELIEF_STOCKLEDGER_H
#define RELIEF_STOCKLEDGER_H

#include "Item.h"
#include "Date.h"
#include <string>
#include <vector>
#include <ostream>

class Camp;

// One line in the stock register: what moved, which way, when and why.
// It belongs to StockLedger and has no life of its own, so it shares the
// header rather than getting a file to itself.
class StockTxn {
    int campId;
    Item item;
    double quantity;
    bool incoming;          // true = stock in, false = stock out
    Date date;
    std::string note;
public:
    StockTxn();
    StockTxn(int campId, Item item, double quantity, bool incoming,
             const Date& date, const std::string& note);
    int getCampId() const;
    Item getItem() const;
    double getQuantity() const;
    bool isIncoming() const;
    const Date& getDate() const;
    const std::string& getNote() const;
    friend std::ostream& operator<<(std::ostream& os, const StockTxn& t);
};

// The stock register. Every change to a camp's stock goes through here, so the
// paper trail and the running total can never disagree - netFor() recomputes
// the balance from the transactions and must match the camp's Stock.
class StockLedger {
    std::vector<StockTxn> txns;
public:
    StockLedger();
    // Both throw std::invalid_argument for a quantity that is not above zero:
    // Stock itself does not check, and a negative "take" would add stock.
    void stockIn(Camp& camp, Item item, double quantity,
                 const Date& date, const std::string& note);
    // Throws InsufficientStockException and logs nothing if the camp is short.
    void stockOut(Camp& camp, Item item, double quantity,
                  const Date& date, const std::string& note);
    int size() const;
    const StockTxn& at(int index) const;           // throws std::out_of_range
    std::vector<StockTxn> forCamp(int campId) const;
    double netFor(int campId, Item item) const;
    void loadTxn(const StockTxn& t);               // CSV loading
    void clear();
};

#endif
