#ifndef RELIEF_ITEM_H
#define RELIEF_ITEM_H

#include <string>

// The relief items a camp stocks and hands out.
enum class Item { Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin };

const int ITEM_COUNT = 7;

std::string itemName(Item it);
std::string itemUnit(Item it);
int itemIndex(Item it);
Item itemAt(int index);
Item itemFromName(const std::string& name);   // throws std::invalid_argument

#endif
