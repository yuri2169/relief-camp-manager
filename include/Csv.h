#ifndef RELIEF_CSV_H
#define RELIEF_CSV_H

#include <string>
#include <vector>

// Field-level CSV handling, kept apart from CsvStore so the fiddly quoting
// rules can be tested on their own.
//
// These files get opened in a spreadsheet and hand-edited by camp staff, so a
// bad field has to name its file and line rather than silently becoming a zero.
// [Static member functions] no Csv object is ever needed: every helper is a
// static function of the class.
class Csv {
public:
    // Splits one record into fields. Empty fields are kept. A field wrapped in
    // double quotes may contain commas, and "" inside quotes means one quote.
    static std::vector<std::string> split(const std::string& line);
    static std::string join(const std::vector<std::string>& fields);
    static std::string escape(const std::string& field);

    // Throw FileFormatException naming the file and line on bad input.
    static int toInt(const std::string& text, const std::string& file, int line);
    static double toDouble(const std::string& text, const std::string& file, int line);
    static void require(int actualFields, int expectedFields,
                        const std::string& file, int line);
};

#endif
