#ifndef RELIEF_DATE_H
#define RELIEF_DATE_H

#include <string>
#include <ostream>

// A plain calendar date. Day-number arithmetic keeps the 3-day health window
// and the "same day" duplicate check simple and testable.
class Date {
    int year;
    int month;
    int day;
public:
    Date();                                // 1970-01-01
    Date(int year, int month, int day);    // parameterised
    Date(const Date& other);               // copy constructor
    ~Date();

    // [Const member functions]
    int getYear() const;
    int getMonth() const;
    int getDay() const;
    bool isValid() const;
    std::string toString() const;          // "YYYY-MM-DD"
    long toDayNumber() const;              // days since 1970-01-01
    Date plusDays(int n) const;            // [Returning objects]

    static Date parse(const std::string& text);   // throws std::invalid_argument
    static Date fromDayNumber(long dayNumber);
    static bool isLeapYear(int year);
    static int daysInMonth(int year, int month);

    // [Friend functions] free-standing operators
    friend bool operator==(const Date& a, const Date& b);
    friend bool operator!=(const Date& a, const Date& b);
    friend bool operator<(const Date& a, const Date& b);
    friend bool operator<=(const Date& a, const Date& b);
    friend long operator-(const Date& a, const Date& b);   // [Binary operator] days between
    friend std::ostream& operator<<(std::ostream& os, const Date& d);
};

#endif
