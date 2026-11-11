#include "Exceptions.h"
#include <sstream>

ReliefException::ReliefException(const std::string& message) : message(message) {}

ReliefException::~ReliefException() {}

const char* ReliefException::what() const noexcept {
    return message.c_str();
}

static std::string buildOverCapacityMessage(int campId, int requested, int freeSpace) {
    std::ostringstream oss;
    oss << "camp " << campId << " cannot take " << requested
        << " more people; only " << freeSpace << " space(s) free";
    return oss.str();
}

OverCapacityException::OverCapacityException(int campId, int requested, int freeSpace)
    : ReliefException(buildOverCapacityMessage(campId, requested, freeSpace)),
      campId(campId), requested(requested), freeSpace(freeSpace) {}

int OverCapacityException::getCampId() const { return campId; }
int OverCapacityException::getRequested() const { return requested; }
int OverCapacityException::getFreeSpace() const { return freeSpace; }

static std::string buildInsufficientStockMessage(Item item, double needed, double available) {
    std::ostringstream oss;
    oss << "not enough " << itemName(item) << ": need " << needed << " "
        << itemUnit(item) << ", have " << available;
    return oss.str();
}

InsufficientStockException::InsufficientStockException(Item item, double needed, double available)
    : ReliefException(buildInsufficientStockMessage(item, needed, available)),
      item(item), needed(needed), available(available) {}

Item InsufficientStockException::getItem() const { return item; }
double InsufficientStockException::getNeeded() const { return needed; }
double InsufficientStockException::getAvailable() const { return available; }

static std::string buildDuplicateClaimMessage(int familyId, const Date& date) {
    std::ostringstream oss;
    oss << "family " << familyId << " has already collected rations on " << date.toString();
    return oss.str();
}

DuplicateClaimException::DuplicateClaimException(int familyId, const Date& date)
    : ReliefException(buildDuplicateClaimMessage(familyId, date)),
      familyId(familyId), date(date) {}

int DuplicateClaimException::getFamilyId() const { return familyId; }
const Date& DuplicateClaimException::getDate() const { return date; }

static std::string buildFileFormatMessage(const std::string& file, int line, const std::string& detail) {
    std::ostringstream oss;
    oss << file << " line " << line << ": " << detail;
    return oss.str();
}

FileFormatException::FileFormatException(const std::string& file, int line, const std::string& detail)
    : ReliefException(buildFileFormatMessage(file, line, detail)),
      file(file), line(line) {}

const std::string& FileFormatException::getFile() const { return file; }
int FileFormatException::getLine() const { return line; }
