#ifndef RELIEF_CONSOLEAPP_H
#define RELIEF_CONSOLEAPP_H

#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "TransferAdvisor.h"
#include "ReportGenerator.h"
#include "CsvStore.h"
#include "Autosaver.h"
#include "Date.h"
#include <string>
#include <mutex>
#include <vector>

// The console shell. It owns all the state, drives the menu, and is the ONLY
// place that catches exceptions and prints messages - every class below it
// throws and stays silent.
class ConsoleApp {
    Registry<Camp> camps;
    Registry<Family> families;
    Registry<Volunteer> volunteers;
    DistributionLog distributions;
    StockLedger ledger;
    HealthMonitor health;
    TransferAdvisor advisor;
    ReportGenerator reporter;
    CsvStore store;
    Autosaver autosaver;

    // Guards every mutation of the registries and logs above. The autosave
    // thread takes this same mutex before writing, so a save never sees a
    // half-finished change.
    std::mutex dataMutex;

    Date today;
    bool running;

    // Set when load() failed. loadAll() leaves every container EMPTY on a
    // failure, so saving that empty state would overwrite whatever is still
    // on disk. While true, saves are skipped until the operator confirms with
    // menu option 11. Guarded by dataMutex.
    bool loadFailed;

    // The autosave callback records why a save failed (the Autosaver itself
    // only counts failures). Guarded by dataMutex.
    std::string lastSaveError;
    int failuresReported;           // main thread only

    void showBanner() const;
    void showMainMenu() const;
    void refreshDemand();                  // recompute each camp's daily draw

    void handleListCamps();
    void handleRegisterFamily();
    void handleAdmitFamily();
    void handleDistribute();
    void handleStockView();
    void handleStockIn();
    void handleTransferSuggestions();
    void handleHealthLog();
    void handleSortFamilies();
    void handleReport();
    void handleSaveNow();
    void handleSetDate();
    void handleAddCamp();
    void reportSaveProblems();             // tells the operator about failed autosaves
    void doSave();                         // caller must hold dataMutex

    std::vector<Family*> familiesAtCamp(int campId) const;
    Camp* chooseCamp(const std::string& prompt);
    Family* chooseFamily(const std::string& prompt);
    void suggestAlternativeCamp(const Family& f, int excludeCampId) const;

    int readInt(const std::string& prompt) const;
    double readDouble(const std::string& prompt) const;
    std::string readLine(const std::string& prompt) const;
    bool readYesNo(const std::string& prompt) const;

public:
    explicit ConsoleApp(const std::string& dataDir);
    ~ConsoleApp();

    void load();
    void saveAll();
    void run();

    void setToday(const Date& d);
    const Date& getToday() const;

    Registry<Camp>& campRegistry();
    Registry<Family>& familyRegistry();
    DistributionLog& distributionLog();
    HealthMonitor& healthMonitor();
};

#endif
