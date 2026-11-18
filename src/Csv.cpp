#include "Csv.h"
#include "Exceptions.h"
#include <sstream>
#include <cctype>
#include <cmath>

std::vector<std::string> Csv::split(const std::string& line) {
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (std::size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';         // "" inside quotes is one literal quote
                    i++;
                } else {
                    inQuotes = false;       // the closing quote is not copied
                }
            } else {
                current += c;
            }
        } else if (c == '"') {
            inQuotes = true;                // the opening quote is not copied
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    // The last field is always pushed, so a trailing comma gives a trailing
    // empty field and "a,,b" gives three fields.
    fields.push_back(current);
    return fields;
}

std::string Csv::escape(const std::string& field) {
    bool needsQuotes = false;
    for (std::size_t i = 0; i < field.size(); i++) {
        char c = field[i];
        if (c == ',' || c == '"' || c == '\n' || c == '\r') {
            needsQuotes = true;
        }
    }
    if (!needsQuotes) {
        return field;
    }
    std::string out = "\"";
    for (std::size_t i = 0; i < field.size(); i++) {
        if (field[i] == '"') {
            out += "\"\"";
        } else {
            out += field[i];
        }
    }
    out += "\"";
    return out;
}

std::string Csv::join(const std::vector<std::string>& fields) {
    std::string out;
    for (std::size_t i = 0; i < fields.size(); i++) {
        if (i > 0) {
            out += ",";
        }
        out += escape(fields[i]);
    }
    return out;
}

// Both number parsers insist that the WHOLE text is consumed, so "12abc" and
// "4.5" (for an int) are rejected instead of quietly becoming 12 and 4.
int Csv::toInt(const std::string& text, const std::string& file, int line) {
    if (!text.empty() && !std::isspace(static_cast<unsigned char>(text[0]))) {
        std::istringstream iss(text);
        int value = 0;
        if ((iss >> value) && iss.eof()) {
            return value;
        }
    }
    throw FileFormatException(file, line, "expected a number, found \"" + text + "\"");
}

double Csv::toDouble(const std::string& text, const std::string& file, int line) {
    if (!text.empty() && !std::isspace(static_cast<unsigned char>(text[0]))) {
        std::istringstream iss(text);
        double value = 0.0;
        if ((iss >> value) && iss.eof() && std::isfinite(value)) {
            return value;
        }
    }
    throw FileFormatException(file, line, "expected a number, found \"" + text + "\"");
}

void Csv::require(int actualFields, int expectedFields,
                  const std::string& file, int line) {
    if (actualFields != expectedFields) {
        std::ostringstream oss;
        oss << "expected " << expectedFields << " fields, found " << actualFields;
        throw FileFormatException(file, line, oss.str());
    }
}
