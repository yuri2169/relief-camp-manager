#include "StockLedger.h"
#include "Camp.h"
#include <stdexcept>

// ---------------------------------------------------------------- StockTxn

StockTxn::StockTxn()
    : campId(0), item(Item::Rice), quantity(0.0), incoming(true), date(), note() {}

StockTxn::StockTxn(int campId, Item item, double quantity, bool incoming,
                   const Date& date, const std::string& note)
    : campId(campId), item(item), quantity(quantity), incoming(incoming),
      date(date), note(note) {}

int StockTxn::getCampId() const { return campId; }
Item StockTxn::getItem() const { return item; }
double StockTxn::getQuantity() const { return quantity; }
bool StockTxn::isIncoming() const { return incoming; }
const Date& StockTxn::getDate() const { return date; }
const std::string& StockTxn::getNote() const { return note; }

std::ostream& operator<<(std::ostream& os, const StockTxn& t) {
    os << t.date << " " << (t.incoming ? "IN " : "OUT") << " "
       << itemName(t.item) << " " << t.quantity << " " << itemUnit(t.item)
       << " camp #" << t.campId;
    if (!t.note.empty()) os << " (" << t.note << ")";
    return os;
}

// -------------------------------------------------------------- StockLedger

StockLedger::StockLedger() {}

// Written as "!(q > 0)" so that NaN is refused as well.
static void requirePositive(double quantity) {
    if (!(quantity > 0.0)) {
        throw std::invalid_argument("stock quantity must be above zero");
    }
}

void StockLedger::stockIn(Camp& camp, Item item, double quantity,
                          const Date& date, const std::string& note) {
    requirePositive(quantity);
    camp.stockRef().add(item, quantity);
    txns.push_back(StockTxn(camp.getId(), item, quantity, true, date, note));
}

void StockLedger::stockOut(Camp& camp, Item item, double quantity,
                           const Date& date, const std::string& note) {
    requirePositive(quantity);
    // The camp's Stock validates; if it throws, we never log the line.
    camp.stockRef().take(item, quantity);
    txns.push_back(StockTxn(camp.getId(), item, quantity, false, date, note));
}

int StockLedger::size() const { return static_cast<int>(txns.size()); }

const StockTxn& StockLedger::at(int index) const {
    if (index < 0 || index >= size()) {
        throw std::out_of_range("no such ledger entry");
    }
    return txns[index];
}

std::vector<StockTxn> StockLedger::forCamp(int campId) const {
    std::vector<StockTxn> result;
    for (size_t i = 0; i < txns.size(); i++) {
        if (txns[i].getCampId() == campId) result.push_back(txns[i]);
    }
    return result;
}

double StockLedger::netFor(int campId, Item item) const {
    double net = 0.0;
    for (size_t i = 0; i < txns.size(); i++) {
        if (txns[i].getCampId() != campId || txns[i].getItem() != item) continue;
        net += txns[i].isIncoming() ? txns[i].getQuantity() : -txns[i].getQuantity();
    }
    return net;
}

void StockLedger::loadTxn(const StockTxn& t) { txns.push_back(t); }

void StockLedger::clear() { txns.clear(); }
