#include "Date.h"
#include <iomanip>
#include <sstream>
#include <stdexcept>

Date::Date() : year(1970), month(1), day(1) {}

Date::Date(int y, int m, int d) : year(y), month(m), day(d) {}

Date::Date(const Date& other)
    : year(other.year), month(other.month), day(other.day) {}

Date::~Date() {}

int Date::getYear() const { return year; }
int Date::getMonth() const { return month; }
int Date::getDay() const { return day; }

bool Date::isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int Date::daysInMonth(int y, int m) {
    if (m < 1 || m > 12) {
        return 0;
    }
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m == 2 && isLeapYear(y)) {
        return 29;
    }
    return days[m - 1];
}

bool Date::isValid() const {
    return year >= 1900 && month >= 1 && month <= 12
        && day >= 1 && day <= daysInMonth(year, month);
}

std::string Date::toString() const {
    std::ostringstream out;
    out << std::setw(4) << std::setfill('0') << year << '-'
        << std::setw(2) << std::setfill('0') << month << '-'
        << std::setw(2) << std::setfill('0') << day;
    return out.str();
}

long Date::toDayNumber() const {
    long days = 0;
    // Whole years between 1970 and this year (counted up or down).
    for (int y = 1970; y < year; y++) {
        days += isLeapYear(y) ? 366 : 365;
    }
    for (int y = 1969; y >= year; y--) {
        days -= isLeapYear(y) ? 366 : 365;
    }
    // Whole months already passed in this year.
    for (int m = 1; m < month; m++) {
        days += daysInMonth(year, m);
    }
    return days + (day - 1);
}

Date Date::fromDayNumber(long dayNumber) {
    int y = 1970;
    // Walk the year forward or backward until the remainder fits in one year.
    while (dayNumber < 0) {
        y--;
        dayNumber += isLeapYear(y) ? 366 : 365;
    }
    while (dayNumber >= (isLeapYear(y) ? 366 : 365)) {
        dayNumber -= isLeapYear(y) ? 366 : 365;
        y++;
    }
    int m = 1;
    while (dayNumber >= daysInMonth(y, m)) {
        dayNumber -= daysInMonth(y, m);
        m++;
    }
    return Date(y, m, static_cast<int>(dayNumber) + 1);
}

Date Date::plusDays(int n) const {
    return fromDayNumber(toDayNumber() + n);
}

Date Date::parse(const std::string& text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        throw std::invalid_argument("bad date: " + text);
    }
    for (std::size_t i = 0; i < text.size(); i++) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            throw std::invalid_argument("bad date: " + text);
        }
    }
    int y = std::stoi(text.substr(0, 4));
    int m = std::stoi(text.substr(5, 2));
    int d = std::stoi(text.substr(8, 2));
    Date result(y, m, d);
    if (!result.isValid()) {
        throw std::invalid_argument("bad date: " + text);
    }
    return result;
}

bool operator==(const Date& a, const Date& b) {
    return a.year == b.year && a.month == b.month && a.day == b.day;
}

bool operator!=(const Date& a, const Date& b) {
    return !(a == b);
}

bool operator<(const Date& a, const Date& b) {
    if (a.year != b.year) return a.year < b.year;
    if (a.month != b.month) return a.month < b.month;
    return a.day < b.day;
}

bool operator<=(const Date& a, const Date& b) {
    return a < b || a == b;
}

long operator-(const Date& a, const Date& b) {
    return a.toDayNumber() - b.toDayNumber();
}

std::ostream& operator<<(std::ostream& os, const Date& d) {
    os << d.toString();
    return os;
}
