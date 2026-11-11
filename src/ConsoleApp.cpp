#include "ConsoleApp.h"
#include "Algorithms.h"
#include "Allocation.h"
#include "DistributionQueue.h"
#include "FamilySorter.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
#include "Resident.h"
#include "RationCard.h"
#include "Item.h"
#include "Symptom.h"
#include "Exceptions.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <cmath>
#include <stdexcept>

namespace {

// The autosave interval lives here and nowhere else: Autosaver's own default
// is overwritten by start(), so this is the one place the 30 seconds exists.
const int kAutosaveSeconds = 30;

// Days of supply at or below this is flagged LOW STOCK.
const double kLowStockDays = 3.0;

// The menu shows numbered choices but FamilySorter::metricByName takes names.
const char* const kMetricNames[] = { "vulnerability", "size", "age", "infants" };
const int kMetricCount = 4;

// One decimal place, so "11.4286" reads as "11.4".
std::string oneDecimal(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value;
    return out.str();
}

}  // namespace

// ------------------------------------------------------------ construction

ConsoleApp::ConsoleApp(const std::string& dataDir)
    : store(dataDir), today(2026, 7, 14), running(false),
      loadFailed(true), lastSaveError(), failuresReported(0) {}

ConsoleApp::~ConsoleApp() {
    // Called from the main thread, and WITHOUT dataMutex held: the worker may
    // be blocked on that mutex inside a save, and stop() joins it.
    autosaver.stop();
    try {
        saveAll();
    } catch (const std::exception& e) {
        // A destructor must not throw.
        std::cerr << "Final save failed: " << e.what() << "\n";
    }
}

// ---------------------------------------------------------------- input

int ConsoleApp::readInt(const std::string& prompt) const {
    while (true) {
        std::cout << prompt << ": ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return 0;                    // end of input: a piped script is over
        }
        std::istringstream in(line);
        int value = 0;
        std::string leftover;
        if ((in >> value) && !(in >> leftover)) {
            return value;
        }
        std::cout << "Please enter a number.\n";
    }
}

double ConsoleApp::readDouble(const std::string& prompt) const {
    while (true) {
        std::cout << prompt << ": ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return 0.0;
        }
        std::istringstream in(line);
        double value = 0.0;
        std::string leftover;
        if ((in >> value) && !(in >> leftover) && std::isfinite(value)) {
            return value;
        }
        std::cout << "Please enter a number.\n";
    }
}

std::string ConsoleApp::readLine(const std::string& prompt) const {
    std::cout << prompt << ": ";
    std::string line;
    if (!std::getline(std::cin, line)) {
        return "";
    }
    return line;
}

bool ConsoleApp::readYesNo(const std::string& prompt) const {
    while (true) {
        std::cout << prompt << " (y/n): ";
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;                // end of input counts as "no"
        }
        if (line == "y" || line == "Y" || line == "yes") return true;
        if (line == "n" || line == "N" || line == "no") return false;
        std::cout << "Please answer y or n.\n";
    }
}

// --------------------------------------------------------------- display

void ConsoleApp::showBanner() const {
    std::cout << "\n==========================================\n"
              << "  RELIEF CAMP MANAGER\n";
    if (camps.size() > 0) {
        std::cout << "  " << camps.at(0)->getDistrict() << " district\n";
    }
    std::cout << "  Date: " << today << "\n"
              << "  offline - no internet required\n"
              << "==========================================\n";
}

void ConsoleApp::showMainMenu() const {
    std::cout << "\n--- MAIN MENU (" << today << ") ---\n"
              << " 1. List camps\n"
              << " 2. Register a family\n"
              << " 3. Admit a family to a camp\n"
              << " 4. Distribute rations\n"
              << " 5. View stock\n"
              << " 6. Receive stock\n"
              << " 7. Transfer suggestions\n"
              << " 8. Log a health case\n"
              << " 9. Sort families\n"
              << "10. Daily report\n"
              << "11. Save now\n"
              << "12. Set today's date\n"
              << "13. Add a camp\n"
              << " 0. Quit\n";
}

// ------------------------------------------------------------ persistence

void ConsoleApp::load() {
    std::lock_guard<std::mutex> lock(dataMutex);
    // loadFailed starts true and is cleared only on success, so ANY escape
    // from loadAll (named or not) leaves saving switched off.
    try {
        store.loadAll(camps, families, volunteers, distributions, ledger, health);
        loadFailed = false;
        std::cout << "Loaded " << camps.size() << " camps and "
                  << families.size() << " families from " << store.getDir() << "\n";
    } catch (const FileFormatException& e) {
        // loadAll clears every container before it rethrows, so nothing at
        // all is loaded now. Say so, and do not let a save overwrite the files.
        loadFailed = true;
        std::cout << "WARNING: " << e.what() << "\n"
                  << "NOTHING was loaded: all camps, families and records are empty.\n"
                  << "The files on disk are untouched and saving is switched off.\n"
                  << "Fix the file and restart, or use option 11 to overwrite it.\n";
    }
    refreshDemand();
}

// Caller must hold dataMutex.
void ConsoleApp::doSave() {
    if (loadFailed) {
        std::cout << "Save skipped: the last load failed and the files on disk "
                  << "were left alone.\n";
        return;
    }
    store.saveAll(camps, families, volunteers, distributions, ledger, health);
}

void ConsoleApp::saveAll() {
    std::lock_guard<std::mutex> lock(dataMutex);
    doSave();
}

void ConsoleApp::reportSaveProblems() {
    int failures = autosaver.getFailureCount();
    if (failures > failuresReported) {
        std::string why;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            why = lastSaveError;
        }
        std::cout << "\n  WARNING: " << (failures - failuresReported)
                  << " autosave(s) failed. Last error: " << why << "\n";
        failuresReported = failures;
    }
}

// Caller must hold dataMutex.
void ConsoleApp::refreshDemand() {
    CsvStore::deriveDailyDemand(camps, families);
}

// ----------------------------------------------------------------- choosers

std::vector<Family*> ConsoleApp::familiesAtCamp(int campId) const {
    std::vector<Family*> result;
    std::vector<Family*> everyone = families.all();
    for (size_t i = 0; i < everyone.size(); i++) {
        if (everyone[i]->getCampId() == campId) {
            result.push_back(everyone[i]);
        }
    }
    return result;
}

Camp* ConsoleApp::chooseCamp(const std::string& prompt) {
    std::vector<Camp*> all = camps.all();
    if (all.empty()) {
        std::cout << "There are no camps yet. Use option 13 to add one.\n";
        return 0;
    }
    for (size_t i = 0; i < all.size(); i++) {
        std::cout << "  " << *all[i] << "\n";
    }
    int id = readInt(prompt + " (camp id, 0 = cancel)");
    if (id == 0) return 0;
    Camp* camp = camps.findById(id);
    if (camp == 0) {
        std::cout << "No camp with id " << id << ".\n";
    }
    return camp;
}

Family* ConsoleApp::chooseFamily(const std::string& prompt) {
    std::vector<Family*> all = families.all();
    if (all.empty()) {
        std::cout << "No families are registered yet. Use option 2.\n";
        return 0;
    }
    for (size_t i = 0; i < all.size(); i++) {
        const Family* f = all[i];
        std::cout << "  #" << f->getId() << " " << f->getHeadName() << " (" << f->getVillage()
                  << "), " << f->size() << " people, camp "
                  << (f->getCampId() == 0 ? std::string("none") : std::to_string(f->getCampId()))
                  << "\n";
    }
    int id = readInt(prompt + " (family id, 0 = cancel)");
    if (id == 0) return 0;
    Family* f = families.findById(id);
    if (f == 0) {
        std::cout << "No family with id " << id << ".\n";
    }
    return f;
}

void ConsoleApp::suggestAlternativeCamp(const Family& f, int excludeCampId) const {
    bool any = false;
    std::vector<Camp*> all = camps.all();
    for (size_t i = 0; i < all.size(); i++) {
        if (all[i]->getId() != excludeCampId && all[i]->freeSpace() >= f.size()) {
            if (!any) std::cout << "  Camps with room for " << f.size() << " people:\n";
            std::cout << "    " << *all[i] << "  (free: " << all[i]->freeSpace() << ")\n";
            any = true;
        }
    }
    if (!any) {
        std::cout << "  No other camp has room for " << f.size() << " people.\n";
    }
}

// ----------------------------------------------------------------- handlers

// Read-only: takes no lock. That is safe only because the main thread is the
// sole mutator and CsvStore::saveAll is const, so the autosave thread merely reads.
void ConsoleApp::handleListCamps() {
    std::vector<Camp*> all = camps.all();
    if (all.empty()) {
        std::cout << "No camps yet. Use option 13 to add one.\n";
        return;
    }
    for (size_t i = 0; i < all.size(); i++) {
        double days = all[i]->getStock().daysOfSupply();
        std::cout << *all[i] << "\n    shelters: " << all[i]->shelterCount() << ", days of supply: ";
        if (days >= Stock::kUnlimitedDays) {
            std::cout << "no demand";
        } else {
            std::cout << oneDecimal(days);
        }
        if (days <= kLowStockDays) std::cout << "   LOW STOCK";
        std::cout << "\n";
    }
}

void ConsoleApp::handleAddCamp() {
    std::string name = readLine("Camp name (blank = cancel)");
    if (name.empty()) return;
    std::string district = readLine("District");
    int capacity = readInt("Capacity (people)");
    if (capacity <= 0) {
        std::cout << "Capacity must be a positive number. Nothing added.\n";
        return;
    }
    std::unique_ptr<Camp> camp(new Camp(name, district, capacity));
    while (true) {
        std::string label = readLine("Shelter label (blank = done)");
        if (label.empty()) break;
        int shelterCap = readInt("Shelter capacity");
        if (shelterCap <= 0) {
            std::cout << "Shelter capacity must be positive; shelter skipped.\n";
            continue;
        }
        camp->addShelter(Camp::Shelter(label, shelterCap));
    }
    std::lock_guard<std::mutex> lock(dataMutex);
    Camp* added = camps.add(std::move(camp));
    std::cout << "Added: " << *added << "\n";
}

void ConsoleApp::handleRegisterFamily() {
    std::string head = readLine("Head of family (blank = cancel)");
    if (head.empty()) return;
    std::string village = readLine("Village");
    // Built privately first; it joins the shared registry only when complete.
    std::unique_ptr<Family> f(new Family(head, village));

    bool another = true;
    while (another) {
        std::string name = readLine("Member name (blank = finish)");
        if (name.empty()) break;
        int age = readInt("Age");
        if (age < 0 || age > 120) {
            std::cout << "Age must be 0 to 120; member skipped.\n";
            continue;
        }
        std::string sexText = readLine("Sex (M/F/O)");
        char sex = sexText.empty() ? 'O' : sexText[0];
        if (sex == 'm') sex = 'M';
        if (sex == 'f') sex = 'F';
        if (sex != 'M' && sex != 'F') sex = 'O';
        bool pregnant = readYesNo("Pregnant?");
        bool disabled = readYesNo("Disabled?");
        bool lactating = readYesNo("Breastfeeding?");

        std::unique_ptr<Person> p;
        if (age <= 2) {
            p.reset(new Infant(name, age, sex, f->getId(), 0));
        } else if (age <= 12) {
            p.reset(new Child(name, age, sex, f->getId(), 0));
        } else if (age < 60) {
            if (readYesNo("Will they take a camp duty?")) {
                std::string skill = readLine("Skill (e.g. medic, cook, driver)");
                std::string shift = readLine("Shift (e.g. morning, evening)");
                p.reset(new ResidentVolunteer(name, age, sex, f->getId(), 0, skill, shift));
            } else {
                p.reset(new Adult(name, age, sex, f->getId(), 0));
            }
        } else {
            p.reset(new Elderly(name, age, sex, f->getId(), 0));
        }
        p->setFlags(pregnant, disabled, lactating);
        f->addMember(p.get());            // the family owns the person from here
        p.release();
        another = readYesNo("Add another member?");
    }
    if (f->size() == 0) {
        std::cout << "A family needs at least one member. Nothing registered.\n";
        return;
    }
    std::lock_guard<std::mutex> lock(dataMutex);
    Family* added = families.add(std::move(f));
    std::cout << "\n" << static_cast<RationCard>(*added).render() << "\n"
              << "Family id " << added->getId() << ", vulnerability score "
              << added->vulnerabilityScore() << "\n";
}

void ConsoleApp::handleAdmitFamily() {
    Family* f = chooseFamily("Admit which family");
    if (f == 0) return;
    if (f->getCampId() != 0) {
        std::cout << "Family " << f->getId() << " is already admitted to camp "
                  << f->getCampId() << ".\n";
        return;
    }
    Camp* camp = chooseCamp("Admit to which camp");
    if (camp == 0) return;

    std::lock_guard<std::mutex> lock(dataMutex);
    try {
        // Find a shelter for the whole family BEFORE touching the camp, so a
        // refusal leaves the camp-level and shelter-level counts in step.
        int shelterIndex = camp->findShelterFor(f->size());
        if (camp->shelterCount() > 0 && shelterIndex < 0 && camp->freeSpace() >= f->size()) {
            std::cout << "\n  REFUSED: no single shelter in " << camp->getName()
                      << " has room for " << f->size() << " people together.\n";
            suggestAlternativeCamp(*f, camp->getId());
            return;
        }
        camp->admit(*f);                  // camp head-count: throws OverCapacityException
        if (shelterIndex >= 0) {
            try {
                camp->shelterAt(shelterIndex).occupy(f->size());
            } catch (const ReliefException&) {
                camp->release(*f);        // undo, keep the two counts together
                // The shelter exception carries camp id -1, so name things ourselves.
                std::cout << "\n  REFUSED: shelter " << camp->shelterAt(shelterIndex).getLabel()
                          << " in " << camp->getName() << " could not take the family.\n";
                return;
            }
        }
        f->setCampId(camp->getId());
        std::string label = shelterIndex >= 0 ? camp->shelterAt(shelterIndex).getLabel() : "";
        for (int i = 0; i < f->size(); i++) {
            Resident* r = dynamic_cast<Resident*>(&f->memberAt(i));
            if (r != 0) r->assignTo(camp->getId(), label);
        }
        refreshDemand();
        std::cout << "Admitted family " << f->getId() << " (" << f->size() << " people) to "
                  << camp->getName();
        if (shelterIndex >= 0) std::cout << ", shelter " << label;
        std::cout << ". Camp now " << camp->getOccupancy() << "/" << camp->getCapacity() << ".\n";
    }
    // [Exception handling] a full camp is expected, so it is handled here where
    // the alternative camps can be offered.
    catch (const OverCapacityException& e) {
        std::cout << "\n  REFUSED: " << e.what() << "\n";
        suggestAlternativeCamp(*f, camp->getId());
    }
}

void ConsoleApp::handleDistribute() {
    Camp* camp = chooseCamp("Distribute at which camp");
    if (camp == 0) return;

    std::lock_guard<std::mutex> lock(dataMutex);
    DistributionQueue queue;
    std::vector<Family*> there = familiesAtCamp(camp->getId());
    for (size_t i = 0; i < there.size(); i++) {
        queue.push(there[i]);
    }
    if (queue.empty()) {
        std::cout << "No families are admitted to " << camp->getName() << ".\n";
        return;
    }
    int served = 0;
    int blocked = 0;
    int shortOfStock = 0;
    while (!queue.empty()) {
        Family* f = queue.pop();
        Allocation alloc(f->getId(), camp->getId(), today, f->dailyRation());
        try {
            // Rehearse the deduction on a copy: if stock is short nothing is
            // logged, and if the claim is a duplicate nothing is deducted.
            Stock trial(camp->getStock());
            trial -= alloc;
            distributions.record(alloc);
            camp->stockRef() -= alloc;
            served++;
            std::cout << "  Served family " << f->getId() << " " << f->getHeadName()
                      << " (vulnerability " << f->vulnerabilityScore() << ")\n";
        }
        catch (const DuplicateClaimException& e) {
            blocked++;
            std::cout << "  BLOCKED: " << e.what() << "\n";
        }
        catch (const InsufficientStockException& e) {
            shortOfStock++;
            std::cout << "  SHORT: family " << f->getId() << " " << f->getHeadName()
                      << " not served - " << e.what() << "\n";
        }
    }
    std::cout << "Round finished at " << camp->getName() << ": " << served << " served, "
              << blocked << " blocked, " << shortOfStock << " short of stock.\n";
}

// Read-only: takes no lock. That is safe only because the main thread is the
// sole mutator and CsvStore::saveAll is const, so the autosave thread merely reads.
void ConsoleApp::handleStockView() {
    Camp* camp = chooseCamp("View stock of which camp");
    if (camp == 0) return;
    std::cout << reporter.stockTable(*camp);
    double days = camp->getStock().daysOfSupply();
    if (days >= Stock::kUnlimitedDays) {
        std::cout << "Days of supply: no daily demand recorded\n";
    } else {
        std::cout << "Days of supply: " << oneDecimal(days) << " (critical item: "
                  << itemName(camp->getStock().mostCriticalItem()) << ")";
        if (days <= kLowStockDays) std::cout << "  LOW";
        std::cout << "\n";
    }
}

void ConsoleApp::handleStockIn() {
    Camp* camp = chooseCamp("Receive stock at which camp");
    if (camp == 0) return;
    for (int i = 0; i < ITEM_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << itemName(itemAt(i)) << " (" << itemUnit(itemAt(i)) << ")\n";
    }
    int choice = readInt("Item number (0 = cancel)");
    if (choice < 1 || choice > ITEM_COUNT) return;
    double qty = readDouble("Quantity");
    std::string note = readLine("Note (who delivered it)");
    std::lock_guard<std::mutex> lock(dataMutex);
    ledger.stockIn(*camp, itemAt(choice - 1), qty, today, note);
    std::cout << "Received " << qty << " " << itemUnit(itemAt(choice - 1)) << " of "
              << itemName(itemAt(choice - 1)) << " at " << camp->getName() << ".\n";
}

void ConsoleApp::handleTransferSuggestions() {
    // The suggest() call only reads (no lock; see the note above handleListCamps).
    std::vector<TransferSuggestion> list = advisor.suggest(camps.all());
    if (list.empty()) {
        std::cout << "No transfers needed.\n";
        return;
    }
    for (size_t i = 0; i < list.size(); i++) {
        std::cout << (i + 1) << ". " << list[i].render() << "\n";
    }
    int pick = readInt("Apply which suggestion (0 = none)");
    if (pick < 1 || pick > static_cast<int>(list.size())) return;
    const TransferSuggestion& s = list[pick - 1];

    std::lock_guard<std::mutex> lock(dataMutex);
    Camp* from = camps.findById(s.getFromCampId());
    Camp* to = camps.findById(s.getToCampId());
    if (from == 0 || to == 0) {
        std::cout << "One of the camps no longer exists.\n";
        return;
    }
    ledger.stockOut(*from, s.getItem(), s.getQuantity(), today, "transfer out");
    ledger.stockIn(*to, s.getItem(), s.getQuantity(), today, "transfer in");
    std::cout << "Moved " << s.getQuantity() << " " << itemUnit(s.getItem()) << " of "
              << itemName(s.getItem()) << " from " << from->getName() << " to "
              << to->getName() << ".\n";
}

void ConsoleApp::handleHealthLog() {
    Camp* camp = chooseCamp("Log a case at which camp");
    if (camp == 0) return;
    for (int i = 0; i < SYMPTOM_COUNT; i++) {
        std::cout << "  " << (i + 1) << ". " << symptomName(static_cast<Symptom>(i)) << "\n";
    }
    int choice = readInt("Symptom number (0 = cancel)");
    if (choice < 1 || choice > SYMPTOM_COUNT) return;
    int count = readInt("Number of cases");
    if (count <= 0) {
        std::cout << "The number of cases must be positive. Nothing logged.\n";
        return;
    }
    Symptom symptom = static_cast<Symptom>(choice - 1);

    std::lock_guard<std::mutex> lock(dataMutex);
    health.logCase(camp->getId(), today, symptom, count);
    int total = health.casesInWindow(camp->getId(), today, symptom);
    std::cout << "Logged " << count << " case(s) of " << symptomName(symptom) << " at "
              << camp->getName() << ". Last " << HealthMonitor::windowDays()
              << " days total: " << total << "\n";
    if (health.outbreakAlert(camp->getId(), today, symptom)) {
        std::cout << "\n  ********************************\n"
                  << "  *** OUTBREAK ALERT ***\n"
                  << "  " << symptomName(symptom) << " at " << camp->getName() << ": "
                  << total << " cases in " << HealthMonitor::windowDays()
                  << " days (threshold " << HealthMonitor::thresholdFor(symptom) << ")\n"
                  << "  ********************************\n";
    }
}

// Read-only: takes no lock. That is safe only because the main thread is the
// sole mutator and CsvStore::saveAll is const, so the autosave thread merely reads.
void ConsoleApp::handleSortFamilies() {
    std::vector<Family*> list = families.all();
    if (list.empty()) {
        std::cout << "No families are registered yet.\n";
        return;
    }
    std::cout << FamilySorter::metricMenu() << "\n";
    int choice = readInt("Sort by (0 = cancel)");
    if (choice < 1 || choice > kMetricCount) return;
    // The menu text is numbered but metricByName only knows names.
    FamilySorter::Metric metric = FamilySorter::metricByName(kMetricNames[choice - 1]);
    FamilySorter::sortDescending(list, metric);
    for (size_t i = 0; i < list.size(); i++) {
        std::cout << "  " << (i + 1) << ". #" << list[i]->getId() << " " << list[i]->getHeadName()
                  << " - " << list[i]->size() << " people, vulnerability "
                  << list[i]->vulnerabilityScore() << "\n";
    }
    Family* worst = findMax(list, &Family::metricVulnerability);
    if (worst != 0) {
        std::cout << "Most vulnerable household: #" << worst->getId() << " "
                  << worst->getHeadName() << " (score " << worst->vulnerabilityScore() << ")\n";
    }
}

// Read-only: takes no lock. That is safe only because the main thread is the
// sole mutator and CsvStore::saveAll is const, so the autosave thread merely reads.
void ConsoleApp::handleReport() {
    Camp* camp = chooseCamp("Report for which camp");
    if (camp == 0) return;
    std::vector<Family*> there = familiesAtCamp(camp->getId());
    std::cout << reporter.dailyReport(*camp, today, there, distributions, health);
    if (readYesNo("Export this report to CSV?")) {
        std::string path = store.getDir() + "/report_" + today.toString() + "_camp"
                           + std::to_string(camp->getId()) + ".csv";
        reporter.exportCsv(path, *camp, today, there, distributions, health);
        std::cout << "Exported to " << path << "\n";
    }
}

void ConsoleApp::handleSaveNow() {
    if (loadFailed) {
        std::cout << "The last load failed, so memory is empty and the files on disk are intact.\n";
        if (!readYesNo("Overwrite the files on disk with the current (empty) state?")) return;
        std::lock_guard<std::mutex> lock(dataMutex);
        loadFailed = false;
    }
    int before = autosaver.getFailureCount();
    // No lock held here: saveNow() takes dataMutex itself.
    autosaver.saveNow();
    if (autosaver.getFailureCount() > before) {
        std::string why;
        {
            std::lock_guard<std::mutex> lock(dataMutex);
            why = lastSaveError;
        }
        failuresReported = autosaver.getFailureCount();
        std::cout << "SAVE FAILED: " << why << "\n";
    } else {
        std::cout << "Saved to " << store.getDir() << ". Save attempts so far: "
                  << autosaver.getSaveCount() << "\n";
    }
}

void ConsoleApp::handleSetDate() {
    std::string text = readLine("New date (YYYY-MM-DD)");
    if (text.empty()) return;
    Date d = Date::parse(text);          // throws std::invalid_argument if malformed
    if (!d.isValid()) {
        std::cout << "That is not a real calendar date.\n";
        return;
    }
    setToday(d);
    std::cout << "Today is now " << today << ".\n";
}

// ----------------------------------------------------------------- the loop

void ConsoleApp::run() {
    running = true;
    showBanner();
    // [Multithreading] start autosaving every kAutosaveSeconds, sharing dataMutex
    autosaver.start(kAutosaveSeconds, dataMutex, [this]() {
        // Runs on the worker thread with dataMutex already held by the saver.
        try {
            if (!loadFailed) {
                store.saveAll(camps, families, volunteers, distributions, ledger, health);
            }
        } catch (const std::exception& e) {
            lastSaveError = e.what();
            throw;                       // so the Autosaver counts the failure
        }
    });
    std::cout << "Autosave is running every " << kAutosaveSeconds << " seconds.\n";

    while (running) {
        reportSaveProblems();
        showMainMenu();
        int choice = readInt("Choose");
        try {
            switch (choice) {
                case 1:  handleListCamps();           break;
                case 2:  handleRegisterFamily();      break;
                case 3:  handleAdmitFamily();         break;
                case 4:  handleDistribute();          break;
                case 5:  handleStockView();           break;
                case 6:  handleStockIn();             break;
                case 7:  handleTransferSuggestions(); break;
                case 8:  handleHealthLog();           break;
                case 9:  handleSortFamilies();        break;
                case 10: handleReport();              break;
                case 11: handleSaveNow();             break;
                case 12: handleSetDate();             break;
                case 13: handleAddCamp();             break;
                case 0:  running = false;             break;
                default: std::cout << "No such option.\n";
            }
        }
        // [Exception handling] every error path in the program ends here.
        // Catch the precise types first, because each one can offer a
        // different recovery.
        catch (const OverCapacityException& e) {
            std::cout << "\n  REFUSED: " << e.what() << "\n";
        }
        catch (const DuplicateClaimException& e) {
            std::cout << "\n  BLOCKED: " << e.what() << "\n"
                      << "  Family " << e.getFamilyId()
                      << " has already been served today. Check the register.\n";
        }
        catch (const InsufficientStockException& e) {
            std::cout << "\n  SHORT: " << e.what() << "\n"
                      << "  Try option 7 for a transfer from another camp.\n";
        }
        catch (const ReliefException& e) {
            std::cout << "\n  ERROR: " << e.what() << "\n";
        }
        catch (const std::invalid_argument& e) {
            std::cout << "\n  INVALID INPUT: " << e.what() << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "\n  UNEXPECTED: " << e.what() << "\n";
        }
    }

    std::cout << "\nStopping autosave and writing final save...\n";
    autosaver.stop();                    // no lock held: the worker may be mid-save
    try {
        saveAll();
        if (!loadFailed) {               // the worker is stopped, so no lock needed
            std::cout << "Saved. " << autosaver.getSaveCount() << " autosave(s) ran this session.\n";
        }
    } catch (const std::exception& e) {
        std::cout << "FINAL SAVE FAILED: " << e.what() << "\n";
    }
}

// ----------------------------------------------------------------- accessors

void ConsoleApp::setToday(const Date& d) { today = d; }
const Date& ConsoleApp::getToday() const { return today; }

Registry<Camp>& ConsoleApp::campRegistry() { return camps; }
Registry<Family>& ConsoleApp::familyRegistry() { return families; }
DistributionLog& ConsoleApp::distributionLog() { return distributions; }
HealthMonitor& ConsoleApp::healthMonitor() { return health; }
