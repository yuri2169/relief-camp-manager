#include "TestRunner.h"
#include "Exceptions.h"
#include <string>
#include <exception>

void run_exception_tests() {
    testing::beginSuite("Custom exception hierarchy");

    // The base carries a message through std::exception::what().
    ReliefException base("something went wrong");
    ASSERT_STR_EQ(std::string(base.what()), "something went wrong");

    // Every custom exception is catchable as the base, and as std::exception.
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), ReliefException);
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), std::exception);
    ASSERT_THROWS(throw InsufficientStockException(Item::Rice, 10.0, 4.0), ReliefException);
    ASSERT_THROWS(throw DuplicateClaimException(101, Date(2026, 7, 14)), ReliefException);
    ASSERT_THROWS(throw FileFormatException("families.csv", 12, "expected 5 fields"), ReliefException);

    // And each one is catchable as its own precise type.
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), OverCapacityException);
    ASSERT_THROWS(throw InsufficientStockException(Item::Rice, 10.0, 4.0), InsufficientStockException);
    ASSERT_THROWS(throw DuplicateClaimException(101, Date(2026, 7, 14)), DuplicateClaimException);
    ASSERT_THROWS(throw FileFormatException("families.csv", 12, "bad"), FileFormatException);

    // OverCapacityException carries what the handler needs to suggest another camp.
    try {
        throw OverCapacityException(3, 5, 2);
    } catch (const OverCapacityException& e) {
        ASSERT_EQ(e.getCampId(), 3);
        ASSERT_EQ(e.getRequested(), 5);
        ASSERT_EQ(e.getFreeSpace(), 2);
        ASSERT_STR_EQ(std::string(e.what()),
                      "camp 3 cannot take 5 more people; only 2 space(s) free");
    }

    // InsufficientStockException names the item and the shortfall.
    try {
        throw InsufficientStockException(Item::Rice, 10.0, 4.0);
    } catch (const InsufficientStockException& e) {
        ASSERT_TRUE(e.getItem() == Item::Rice);
        ASSERT_NEAR(e.getNeeded(), 10.0, 1e-9);
        ASSERT_NEAR(e.getAvailable(), 4.0, 1e-9);
        ASSERT_TRUE(std::string(e.what()).find("Rice") != std::string::npos);
        ASSERT_STR_EQ(std::string(e.what()), "not enough Rice: need 10 kg, have 4");
    }

    // DuplicateClaimException names the family and the date.
    try {
        throw DuplicateClaimException(101, Date(2026, 7, 14));
    } catch (const DuplicateClaimException& e) {
        ASSERT_EQ(e.getFamilyId(), 101);
        ASSERT_TRUE(e.getDate() == Date(2026, 7, 14));
        ASSERT_STR_EQ(std::string(e.what()),
                      "family 101 has already collected rations on 2026-07-14");
    }

    // FileFormatException points at the exact line, which is what makes a
    // hand-edited CSV debuggable.
    try {
        throw FileFormatException("families.csv", 12, "expected 5 fields, found 3");
    } catch (const FileFormatException& e) {
        ASSERT_STR_EQ(e.getFile(), "families.csv");
        ASSERT_EQ(e.getLine(), 12);
        ASSERT_STR_EQ(std::string(e.what()),
                      "families.csv line 12: expected 5 fields, found 3");
    }

    // Catching by base reference keeps the derived message: the virtual what()
    // is doing the work.
    try {
        throw InsufficientStockException(Item::Dal, 3.0, 0.5);
    } catch (const ReliefException& e) {
        ASSERT_TRUE(std::string(e.what()).find("Dal") != std::string::npos);
    }
}
