#include "CsvReader.h"
#include "Csv.h"
#include "Exceptions.h"

bool CsvReader::readPhysicalLine(std::string& text) {
    if (!std::getline(in, text)) {
        return false;
    }
    nextLine++;
    if (!text.empty() && text[text.size() - 1] == '\r') {
        text.erase(text.size() - 1);
    }
    return true;
}

bool CsvReader::hasOpenQuote(const std::string& text) {
    bool open = false;
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] == '"') {
            open = !open;
        }
    }
    return open;
}

bool CsvReader::readRecord(std::vector<std::string>& fields) {
    std::string text;
    do {
        recordLine = nextLine;
        if (!readPhysicalLine(text)) {
            return false;
        }
    } while (text.empty());                     // skip blank lines
    if (recordLine == 1 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
        text.erase(0, 3);
    }
    while (hasOpenQuote(text)) {                // the field continues on the next line
        std::string more;
        if (!readPhysicalLine(more)) {
            throw FileFormatException(file, recordLine, "a quoted field is never closed");
        }
        text += "\n" + more;
    }
    fields = Csv::split(text);
    return true;
}

CsvReader::CsvReader(const std::string& fullPath, const std::string& fileName, int columns)
    : in(fullPath.c_str(), std::ios::binary), file(fileName), expected(columns),
      nextLine(1), recordLine(0) {
    std::vector<std::string> header;
    if (in.is_open() && readRecord(header)) {
        Csv::require(static_cast<int>(header.size()), columns, file, recordLine);
    }
}

bool CsvReader::isOpen() const { return in.is_open(); }
int CsvReader::line() const { return recordLine; }

bool CsvReader::next(std::vector<std::string>& fields) {
    if (!readRecord(fields)) {
        return false;
    }
    Csv::require(static_cast<int>(fields.size()), expected, file, recordLine);
    return true;
}
