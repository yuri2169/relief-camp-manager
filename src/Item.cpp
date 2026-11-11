#include "Item.h"
#include <stdexcept>

std::string itemName(Item it) {
    switch (it) {
        case Item::Rice:      return "Rice";
        case Item::Dal:       return "Dal";
        case Item::Oil:       return "Oil";
        case Item::Salt:      return "Salt";
        case Item::ORS:       return "ORS";
        case Item::BabyFood:  return "Baby food";
        case Item::Tarpaulin: return "Tarpaulin";
    }
    return "Unknown";
}

std::string itemUnit(Item it) {
    switch (it) {
        case Item::Rice:
        case Item::Dal:
        case Item::Salt:
        case Item::BabyFood:  return "kg";
        case Item::Oil:       return "litre";
        case Item::ORS:       return "packets";
        case Item::Tarpaulin: return "sheets";
    }
    return "units";
}

int itemIndex(Item it) {
    return static_cast<int>(it);
}

Item itemAt(int index) {
    if (index < 0 || index >= ITEM_COUNT) {
        throw std::out_of_range("item index out of range");
    }
    return static_cast<Item>(index);
}

Item itemFromName(const std::string& name) {
    for (int i = 0; i < ITEM_COUNT; i++) {
        if (itemName(itemAt(i)) == name) {
            return itemAt(i);
        }
    }
    throw std::invalid_argument("unknown item: " + name);
}
