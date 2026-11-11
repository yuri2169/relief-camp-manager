#include "TestRunner.h"
#include "ReportGenerator.h"
#include "Camp.h"
#include "Family.h"
#include "Infant.h"
#include "Adult.h"
#include "Elderly.h"
#include "DistributionLog.h"
#include "HealthMonitor.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdio>

// True when `text` contains `line` as one complete line.
static bool hasLine(const std::string& text, const std::string& line) {
    std::istringstream in(text);
    std::string current;
    while (std::getline(in, current)) {
        if (current == line) { return true; }
    }
    return false;
}

static bool has(const std::string& text, const std::string& piece) {
    return text.find(piece) != std::string::npos;
}

static std::string readFile(const std::string& path) {
    std::ifstream in(path.c_str());
    ASSERT_TRUE(in.is_open());
    std::string text;
    std::string line;
    while (std::getline(in, line)) { text += line + "\n"; }
    return text;
}

// Counts fields the way a CSV reader does: commas inside quotes and newlines
// inside quotes do not separate anything.
static bool allRowsHaveThreeFields(const std::string& text) {
    int fields = 1;
    bool inQuotes = false;
    int rows = 0;
    for (std::size_t i = 0; i < text.size(); i++) {
        char c = text[i];
        if (c == '"') { inQuotes = !inQuotes; }
        else if (c == ',' && !inQuotes) { fields++; }
        else if (c == '\n' && !inQuotes) {
            if (fields != 3) { return false; }
            fields = 1;
            rows++;
        }
    }
    return rows > 0 && !inQuotes;
}

static std::string num(int n) {
    std::ostringstream os;
    os << n;
    return os.str();
}

void run_report_tests() {
    testing::beginSuite("ReportGenerator");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    Date today(2026, 7, 14);

    Camp camp("Bhuragaon HS School", "Morigaon", 200);
    camp.addShelter(Camp::Shelter("Room-1", 60));
    camp.addShelter(Camp::Shelter("Tent-1", 40));
    camp.stockRef().set(Item::Rice, 500.0);
    camp.stockRef().set(Item::Dal, 60.0);
    camp.stockRef().set(Item::ORS, 300.0);

    Family f1("Bipul Das", "Bhuragaon");
    f1.addMember(new Adult("Bipul Das", 35, 'M', f1.getId(), camp.getId()));
    f1.addMember(new Infant("Mitu Das", 1, 'F', f1.getId(), camp.getId()));
    f1.addMember(new Elderly("Minoti Devi", 72, 'F', f1.getId(), camp.getId()));

    Family f2("Rupa Nath", "Laharighat");
    f2.addMember(new Adult("Rupa Nath", 30, 'F', f2.getId(), camp.getId()));
    f2.addMember(new Adult("Hiren Nath", 33, 'M', f2.getId(), camp.getId()));

    camp.admit(f1);
    camp.admit(f2);
    f1.setCampId(camp.getId());
    f2.setCampId(camp.getId());

    std::vector<Family*> families;
    families.push_back(&f1);
    families.push_back(&f2);

    // The camp's daily draw is the sum of its families' entitlements:
    // rice 1.5, dal 0.24, ORS 0.75 per day, so 500 / 60 / 300 last
    // 333.3 / 250.0 / 400.0 days. Oil, salt and baby food have demand but no
    // stock (0.0 days); tarpaulin has no demand at all.
    Ration demand = f1.dailyRation() + f2.dailyRation();
    camp.stockRef().setDailyDemand(demand);

    DistributionLog log;
    log.record(Allocation(f1.getId(), camp.getId(), today, f1.dailyRation()));

    HealthMonitor health;
    health.logCase(camp.getId(), today, Symptom::Diarrhoea, 11);   // over the threshold

    ReportGenerator reporter;

    // --- the console report ---
    std::string report = reporter.dailyReport(camp, today, families, log, health);

    // Header: private members camp.id, name, district, occupancy, capacity.
    ASSERT_TRUE(hasLine(report, " DAILY CAMP REPORT   2026-07-14"));
    ASSERT_TRUE(hasLine(report, " Camp      : #1  Bhuragaon HS School"));
    ASSERT_TRUE(hasLine(report, " District  : Morigaon"));
    ASSERT_TRUE(hasLine(report, " Occupancy : 5 / 200"));
    ASSERT_FALSE(has(report, "OVER CAPACITY"));

    // Shelters come from the private vector, in order.
    ASSERT_TRUE(hasLine(report, "  Room-1: 0 / 60"));
    ASSERT_TRUE(hasLine(report, "  Tent-1: 0 / 40"));
    ASSERT_TRUE(report.find("Room-1") < report.find("Tent-1"));

    // Stock table rows: label, quantity, unit and days of supply together.
    ASSERT_TRUE(hasLine(report, "  Rice         500.0 kg        333.3 days"));
    ASSERT_TRUE(hasLine(report, "  Dal           60.0 kg        250.0 days"));
    ASSERT_TRUE(hasLine(report, "  ORS          300.0 packets   400.0 days"));
    // Out of stock with real demand is flagged; no demand is not a number.
    ASSERT_TRUE(hasLine(report, "  Oil            0.0 litre       0.0 days  LOW"));
    ASSERT_TRUE(hasLine(report, "  Tarpaulin      0.0 sheets  no daily demand"));
    ASSERT_FALSE(has(report, "999"));
    ASSERT_TRUE(hasLine(report, "Worst item: Oil (0.0 days)"));

    // Distribution: served versus pending, and what went out.
    ASSERT_TRUE(hasLine(report, "Served today: 1 of 2 families"));
    ASSERT_TRUE(hasLine(report, "  Rice: 0.70 kg"));
    ASSERT_TRUE(hasLine(report, "  Dal: 0.12 kg"));

    // Health: the count with its window, and the alert line.
    ASSERT_TRUE(hasLine(report, "  Diarrhoea: 11 cases in the last 3 days"));
    ASSERT_TRUE(hasLine(report, "  *** OUTBREAK ALERT: Diarrhoea ***"));

    // Families: one line each, collected or pending.
    ASSERT_TRUE(hasLine(report, "  #1  Bipul Das  (3 people, vulnerability " +
                                num(f1.vulnerabilityScore()) + ")  collected"));
    ASSERT_TRUE(hasLine(report, "  #2  Rupa Nath  (2 people, vulnerability " +
                                num(f2.vulnerabilityScore()) + ")  PENDING"));

    // A quiet camp has no OUTBREAK banner, which proves the banner is driven
    // by the data and not printed unconditionally.
    HealthMonitor quiet;
    quiet.logCase(camp.getId(), today, Symptom::Diarrhoea, 2);
    std::string calmReport = reporter.dailyReport(camp, today, families, log, quiet);
    ASSERT_FALSE(has(calmReport, "OUTBREAK"));
    ASSERT_TRUE(hasLine(calmReport, "  Diarrhoea: 2 cases in the last 3 days"));

    // No health data at all, and a camp that has no demand recorded.
    HealthMonitor none;
    ASSERT_TRUE(hasLine(reporter.dailyReport(camp, today, families, log, none),
                        "  No cases in the last 3 days"));

    Camp bare("Hall", "Jorhat", 10);
    bare.stockRef().set(Item::Rice, 5.0);
    std::string bareStock = reporter.stockTable(bare);
    ASSERT_TRUE(hasLine(bareStock, "  Rice           5.0 kg      no daily demand"));
    ASSERT_TRUE(hasLine(bareStock, "Worst item: none (no daily demand recorded)"));
    ASSERT_FALSE(has(bareStock, "999"));

    // Large stock against real demand is a capped figure, never "no demand".
    Camp rich("Depot", "Jorhat", 10);
    rich.stockRef().set(Item::Rice, 5000.0);
    rich.stockRef().set(Item::Dal, 100.0);
    Ration smallDemand;
    smallDemand.set(Item::Dal, 10.0);
    smallDemand.set(Item::Salt, 1.0);
    rich.stockRef().setDailyDemand(smallDemand);
    rich.stockRef().set(Item::Salt, 50000.0);
    std::string richTable = reporter.stockTable(rich);
    ASSERT_TRUE(hasLine(richTable, "  Salt" + std::string(7,' ') + "50000.0 kg" + std::string(9,' ') + "999+ days"));
    ASSERT_TRUE(hasLine(richTable, "  Dal" + std::string(10,' ') + "100.0 kg" + std::string(9,' ') + "10.0 days"));
    // Rice has stock but no demand: that one really is "no daily demand".
    ASSERT_TRUE(hasLine(richTable, "  Rice        5000.0 kg      no daily demand"));
    ASSERT_TRUE(hasLine(richTable, "Worst item: Dal (10.0 days)"));
    ASSERT_FALSE(has(richTable, " 999 "));
    // Only well-stocked items with demand: worst item is capped, not "none".
    Camp flush("Depot2", "Jorhat", 10);
    flush.stockRef().set(Item::Salt, 50000.0);
    Ration saltOnly;
    saltOnly.set(Item::Salt, 1.0);
    flush.stockRef().setDailyDemand(saltOnly);
    ASSERT_TRUE(hasLine(reporter.stockTable(flush), "Worst item: Salt (999+ days)"));

    // An empty camp renders a fallback line in every empty section.
    Camp fresh("New Camp", "Jorhat", 10);
    std::vector<Family*> noFamilies;
    std::string emptyReport = reporter.dailyReport(fresh, today, noFamilies, log, none);
    ASSERT_TRUE(hasLine(emptyReport, "  No shelters"));
    ASSERT_TRUE(hasLine(emptyReport, "  Nothing issued today"));
    ASSERT_TRUE(hasLine(emptyReport, "  No cases in the last 3 days"));
    ASSERT_TRUE(hasLine(emptyReport, "  No families"));

    // A camp at exactly its capacity (isOverflowing is occupancy >= capacity) is flagged.
    Camp full("Tent", "Jorhat", 5);
    full.setOccupancy(5);
    ASSERT_TRUE(hasLine(reporter.dailyReport(full, today, families, log, none),
                        " Occupancy : 5 / 5   *** OVER CAPACITY ***"));

    // stockTable on its own, for the stock menu screen.
    std::string table = reporter.stockTable(camp);
    ASSERT_TRUE(hasLine(table, "  Rice         500.0 kg        333.3 days"));
    ASSERT_TRUE(hasLine(table, "Worst item: Oil (0.0 days)"));
    ASSERT_FALSE(has(table, "DAILY CAMP REPORT"));

    // --- CSV export ---
    const std::string path = "test_report_out.csv";
    std::remove(path.c_str());
    reporter.exportCsv(path, camp, today, families, log, health);

    std::ifstream in(path.c_str());
    ASSERT_TRUE(in.is_open());
    std::string contents;
    std::string line;
    int lineCount = 0;
    std::string firstLine;
    while (std::getline(in, line)) {
        if (lineCount == 0) { firstLine = line; }
        contents += line;
        contents += "\n";
        lineCount++;
    }
    in.close();

    ASSERT_STR_EQ(firstLine, "section,key,value");
    ASSERT_TRUE(lineCount >= 4);
    ASSERT_TRUE(hasLine(contents, "report,date,2026-07-14"));
    ASSERT_TRUE(hasLine(contents, "camp,name,Bhuragaon HS School"));
    ASSERT_TRUE(hasLine(contents, "camp,occupancy,5"));
    ASSERT_TRUE(hasLine(contents, "camp,capacity,200"));
    ASSERT_TRUE(hasLine(contents, "shelter,Room-1,0/60"));
    ASSERT_TRUE(hasLine(contents, "shelter,Tent-1,0/40"));
    ASSERT_TRUE(hasLine(contents, "stock,Rice,500"));
    ASSERT_TRUE(hasLine(contents, "stock,Dal,60"));
    ASSERT_TRUE(hasLine(contents, "distribution,families_served,1"));
    ASSERT_TRUE(hasLine(contents, "health,Diarrhoea,11"));
    ASSERT_TRUE(hasLine(contents, "family,Bipul Das,collected"));
    ASSERT_TRUE(hasLine(contents, "family,Rupa Nath,pending"));

    std::remove(path.c_str());

    // Values with a comma, a quote or a newline are quoted, so every row keeps
    // exactly three fields.
    Camp commaCamp("School, East Block", "Morigaon", 20);
    reporter.exportCsv(path, commaCamp, today, families, log, health);
    std::string commaContents = readFile(path);
    ASSERT_TRUE(hasLine(commaContents, "camp,name,\"School, East Block\""));
    ASSERT_TRUE(allRowsHaveThreeFields(commaContents));

    Camp quoteCamp("The \"Big\" Hall", "\"", 20);
    reporter.exportCsv(path, quoteCamp, today, families, log, health);
    std::string quoteContents = readFile(path);
    ASSERT_TRUE(hasLine(quoteContents, "camp,name,\"The \"\"Big\"\" Hall\""));
    ASSERT_TRUE(hasLine(quoteContents, "camp,district,\"\"\"\""));   // a lone quote
    ASSERT_TRUE(allRowsHaveThreeFields(quoteContents));

    Camp newlineCamp("Line one\nLine two", "Morigaon", 20);
    reporter.exportCsv(path, newlineCamp, today, families, log, health);
    std::string newlineContents = readFile(path);
    ASSERT_TRUE(has(newlineContents, "camp,name,\"Line one\nLine two\"\n"));
    ASSERT_TRUE(allRowsHaveThreeFields(newlineContents));
    std::remove(path.c_str());

    // An unwritable path is reported, not swallowed.
    ASSERT_THROWS(reporter.exportCsv("no_such_dir_xyz/report.csv", camp, today,
                                     families, log, health),
                  FileFormatException);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
