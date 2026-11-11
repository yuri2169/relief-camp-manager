#ifndef RELIEF_CSVSTORE_H
#define RELIEF_CSVSTORE_H

#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include <string>

// Saves and loads the whole system as plain CSV.
//
// Plain text is the point: no database server to install, the files open in any
// spreadsheet, a camp clerk can read and fix them by hand, and the data
// survives a power cut. This is the offline-first promise of the project.
//
// The nine files, each with a header row:
//   camps.csv          id,name,district,capacity,occupancy
//   shelters.csv       campId,label,capacity,occupancy
//   stock.csv          campId,item,quantity
//   families.csv       id,headName,village,campId
//   members.csv        person_id,familyId,typeTag,name,age,sex,pregnant,disabled,lactating,skill,shift
//   volunteers.csv     id,name,age,sex,skill,shift,organisation
//   distributions.csv  familyId,campId,date, then one column per item (enum order)
//   stock_txns.csv     campId,item,quantity,incoming,date,note
//   health.csv         campId,date,symptom,count
//
// A camp's daily demand is not stored: stock.csv holds quantities only, and
// demand is derived from the camp's families on load. A hand-set demand that
// differs from that sum will not survive a reload.
// Saving writes every file to "<name>.tmp" and renames them into place only
// when all nine were written, so a failed save leaves the old files intact.
//
// [File handling] One file per kind of record, each with a header row.
class CsvStore {
    std::string dir;

    // Each of these writes a temporary file and throws FileFormatException if it fails.
    void saveCamps(const Registry<Camp>& camps) const;
    void saveShelters(const Registry<Camp>& camps) const;
    void saveStock(const Registry<Camp>& camps) const;
    void saveFamilies(const Registry<Family>& families) const;
    void saveMembers(const Registry<Family>& families) const;
    void saveVolunteers(const Registry<Volunteer>& volunteers) const;
    void saveDistributions(const DistributionLog& log) const;
    void saveStockTxns(const StockLedger& ledger) const;
    void saveHealth(const HealthMonitor& health) const;

    // Each of these throws FileFormatException on malformed input. A missing
    // file is not an error: the loader simply loads nothing.
    void loadCamps(Registry<Camp>& camps) const;
    void loadShelters(Registry<Camp>& camps) const;
    void loadStock(Registry<Camp>& camps) const;
    void loadFamilies(const Registry<Camp>& camps, Registry<Family>& families) const;
    void loadMembers(const Registry<Volunteer>& volunteers, Registry<Family>& families) const;
    void loadVolunteers(Registry<Volunteer>& volunteers) const;
    void loadDistributions(DistributionLog& log) const;
    void loadStockTxns(StockLedger& ledger) const;
    void loadHealth(HealthMonitor& health) const;

    std::string path(const std::string& fileName) const;

public:
    explicit CsvStore(const std::string& dir);
    const std::string& getDir() const;
    bool dataExists() const;        // true when camps.csv is readable

    void saveAll(const Registry<Camp>& camps,
                 const Registry<Family>& families,
                 const Registry<Volunteer>& volunteers,
                 const DistributionLog& log,
                 const StockLedger& ledger,
                 const HealthMonitor& health) const;

    // Replaces the contents of everything passed in. If the files are
    // malformed it throws FileFormatException and every container is left
    // empty, never half-loaded.
    void loadAll(Registry<Camp>& camps,
                 Registry<Family>& families,
                 Registry<Volunteer>& volunteers,
                 DistributionLog& log,
                 StockLedger& ledger,
                 HealthMonitor& health) const;

    // [Runtime polymorphism from data] turns a type tag back into the right
    // concrete Person. This is the one place that knows every subclass, so
    // adding a person type means touching exactly one switch.
    static Person* makePerson(const std::string& typeTag, const std::string& name,
                              int age, char sex, int familyId, int campId,
                              const std::string& skill, const std::string& shift);
};

#endif
