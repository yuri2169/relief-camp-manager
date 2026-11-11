#ifndef RELIEF_CSVREADER_H
#define RELIEF_CSVREADER_H

#include <fstream>
#include <string>
#include <vector>

// Reads one CSV file record by record, for CsvStore. The header is line 1; the
// first data row is line 2. A quoted field may contain a line break, so one
// record can cover several physical lines; its number is the line it STARTS
// on. Blank lines are skipped, a trailing '\r' (a file saved on Windows) is
// ignored, and so is a spreadsheet's byte-order mark. A missing last newline
// is fine. Malformed framing throws FileFormatException naming file and line.
class CsvReader {
    std::ifstream in;
    std::string file;
    int expected;       // columns every row must have
    int nextLine;       // number of the next physical line to read
    int recordLine;     // line the most recent record started on

    bool readPhysicalLine(std::string& text);
    static bool hasOpenQuote(const std::string& text);
    bool readRecord(std::vector<std::string>& fields);

public:
    // Opens the file and checks the header has `columns` columns. A file that
    // cannot be opened is not an error: isOpen() reports it.
    CsvReader(const std::string& fullPath, const std::string& fileName, int columns);

    bool isOpen() const;
    int line() const;                 // line the last record started on

    // The next data row, already checked to have the right number of fields.
    bool next(std::vector<std::string>& fields);
};

#endif
