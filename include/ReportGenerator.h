#ifndef RELIEF_REPORTGENERATOR_H
#define RELIEF_REPORTGENERATOR_H

#include "Date.h"
#include <string>
#include <vector>

class Camp;
class Family;
class DistributionLog;
class HealthMonitor;

// Builds the daily camp summary the circle office asks for, on the console and
// as a CSV an official can attach to an email once the network is back.
//
// Camp declares this class a friend, so the report reads camp.shelters,
// camp.occupancy and camp.stock directly. Camp has no accessor that hands out
// its whole shelter list, and adding one purely for the report would widen
// Camp's interface for every other caller too.
//
// Every parameter is a const reference: generating a report must never change
// what it reports on.
class ReportGenerator {
public:
    ReportGenerator();

    std::string dailyReport(const Camp& camp,
                            const Date& date,
                            const std::vector<Family*>& families,
                            const DistributionLog& log,
                            const HealthMonitor& health) const;

    std::string stockTable(const Camp& camp) const;

    // Throws FileFormatException when the file cannot be opened for writing.
    void exportCsv(const std::string& path,
                   const Camp& camp,
                   const Date& date,
                   const std::vector<Family*>& families,
                   const DistributionLog& log,
                   const HealthMonitor& health) const;
};

#endif
