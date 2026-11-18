#ifndef RELIEF_EXCEPTIONS_H
#define RELIEF_EXCEPTIONS_H

#include "Item.h"
#include "Date.h"
#include <exception>
#include <string>

// Everything this program throws derives from
// here, so the menu can write one `catch (const ReliefException& e)` and still
// recover the precise type when it needs the details.
class ReliefException : public std::exception {
    std::string message;
public:
    explicit ReliefException(const std::string& message);
    ~ReliefException() override;
    const char* what() const noexcept override;
};

// Thrown by Camp::admit and Camp::Shelter::occupy.
// It carries the free space so the handler can suggest another camp.
class OverCapacityException : public ReliefException {
    int campId;
    int requested;
    int freeSpace;
public:
    OverCapacityException(int campId, int requested, int freeSpace);
    int getCampId() const;
    int getRequested() const;
    int getFreeSpace() const;
};

// Thrown by Stock::take and operator-(Stock, Allocation).
class InsufficientStockException : public ReliefException {
    Item item;
    double needed;
    double available;
public:
    InsufficientStockException(Item item, double needed, double available);
    Item getItem() const;
    double getNeeded() const;
    double getAvailable() const;
};

// Thrown by DistributionLog::record when a family tries to
// claim twice on the same day, in any camp.
class DuplicateClaimException : public ReliefException {
    int familyId;
    Date date;
public:
    DuplicateClaimException(int familyId, const Date& date);
    int getFamilyId() const;
    const Date& getDate() const;
};

// Thrown by the CSV layer, with the file and line number
// so a hand-edited data file can be fixed.
class FileFormatException : public ReliefException {
    std::string file;
    int line;
public:
    FileFormatException(const std::string& file, int line, const std::string& detail);
    const std::string& getFile() const;
    int getLine() const;
};

#endif
