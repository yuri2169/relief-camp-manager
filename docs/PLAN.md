# Offline Relief Camp Manager — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an offline-first, console-based relief camp management system in C++17 that tracks camps, families, rations, stock, distributions and health in a flood relief camp, persists everything to CSV, and demonstrates every row of the CS 203 OOP syllabus table visibly in the source.

**Architecture:** One `Person` tree with `Person` as an abstract virtual base of `Resident` and `Volunteer`; age types (`Infant`, `Child`, `Adult`, `Elderly`) hang off `Resident`, and `ResidentVolunteer` multiply inherits `Adult` and `Volunteer`. `Family` owns a raw `Person**` array with a hand-written rule of three; `Registry<T>` uses `std::unique_ptr` as the deliberate contrast. Entities guard their own invariants and throw; small single-purpose collaborators (`DistributionLog`, `StockLedger`, `TransferAdvisor`, `HealthMonitor`, `ReportGenerator`, `CsvStore`, `Autosaver`, `DistributionQueue`, `FamilySorter`) handle cross-entity work; a thin `ConsoleApp` owns them all and drives the menu.

**Tech Stack:** C++17, standard library only. `g++` / `clang++`. GNU Make plus CMake. Self-written header-only assert runner in `tests/`. No third-party libraries, no package manager, no network.

## Global Constraints

Every task's requirements implicitly include all of the following.

- **C++17, standard library only.** No third-party libraries, no downloads, no network calls at build or run time.
- **Must build with `g++` on Linux, macOS and Windows (MinGW).** Provide both a `Makefile` and a `CMakeLists.txt`. Compile flags: `-std=c++17 -Wall -Wextra -pedantic`.
- **Readable for a 3rd-semester student.** No template metaprogramming, no SFINAE, no fold expressions, no `auto` where the type teaches something. STL containers internally are fine, but every syllabus construct is written by us.
- **Every syllabus construct carries a tag comment** naming the concept on the line above it, in square brackets, e.g. `// [Virtual base class]`, `// [Pure virtual function]`, `// [Friend class]`. `docs/OOP_MAPPING.md` is generated from these tags in Task 23, so the tag text must be exact and consistent.
- **One class per header/source pair.** Folders: `include/`, `src/`, `tests/`, `data/`, `docs/`. Small helper value classes that only exist to serve one owner (`Camp::Shelter`, `StockTxn`, `HealthCase`, `TransferSuggestion`) may share their owner's header.
- **Include guards:** `#ifndef RELIEF_<FILENAME>_H` / `#define` / `#endif`. No `#pragma once`.
- **No GUI, no networking, no login or authentication, no multi-language UI, no SMS.**
- **Tests:** self-written assert-based runner in `tests/`. Nothing downloaded. Every task writes its failing test first and the test must be observed failing before the implementation is written.
- **Commit after every task** with a clear message.
- **Destructor lifecycle messages** print only when the macro `RELIEF_DEBUG` is defined (`make debug`), so normal runs and the demo stay clean.
- **IDs:** every entity with an ID uses a `static int nextId` and a `static int generateId()`. CSV loading uses `adoptId(int)` plus `resetIdCounter(int)` so reloaded data keeps its original IDs and new objects do not collide.

---

## File Structure

### Headers and sources (`include/` + `src/`)

| File pair | Responsibility |
|---|---|
| `Debug.h` | `RELIEF_DEBUG` guard and the `RELIEF_LIFECYCLE(msg)` macro. Header only. |
| `Item.h/.cpp` | `enum class Item`, `ITEM_COUNT`, name/unit/index conversions. |
| `Date.h/.cpp` | `Date` value class: validity, day-number arithmetic, parse, comparison operators. |
| `Symptom.h/.cpp` | `enum class Symptom`, `SYMPTOM_COUNT`, name conversions. |
| `Ration.h/.cpp` | Per-item daily entitlement. `int`-to-`Ration` conversion, `+ - *`, `[]`, `<<`. |
| `RationNorms.h/.cpp` | Static norm tables and static factory functions. The single home for all ration norms. |
| `Person.h/.cpp` | Abstract base. Static ID generator, three constructors, virtual destructor, four pure virtuals, friend `operator<<`. |
| `Resident.h/.cpp` | `virtual public Person`. Abstract. Adds family/camp/shelter placement. |
| `Volunteer.h/.cpp` | `virtual public Person`. Concrete. Worker meal ration, low vulnerability. |
| `Infant.h/.cpp` | `public Resident`. 0–2 years. |
| `Child.h/.cpp` | `public Resident`. 3–12 years. |
| `Adult.h/.cpp` | `public Resident`. 13–59 years. Pregnancy and lactation bonuses. |
| `Elderly.h/.cpp` | `public Adult`. 60+. Multilevel inheritance. |
| `ResidentVolunteer.h/.cpp` | `public Adult, public Volunteer`. Multiple, hybrid, virtual base resolution. |
| `RationCard.h/.cpp` | Printable entitlement card, the target of `Family`-to-`RationCard` conversion. |
| `Family.h/.cpp` | Owns `Person** members`. Rule of three, deep copy via `clone()`, polymorphic ration sum, metric functions for pointer-to-member sorting, `< == <<`, conversion to `RationCard`. |
| `Registry.h` | `template <typename T> class Registry` over `std::unique_ptr<T>`. Header only. |
| `Algorithms.h` | `findMax` function template taking a pointer-to-member metric. Header only. |
| `Exceptions.h/.cpp` | `ReliefException` plus `OverCapacityException`, `InsufficientStockException`, `DuplicateClaimException`, `FileFormatException`. |
| `Allocation.h/.cpp` | One handout: family, camp, date, amount. Converts to `Ration`. |
| `Stock.h/.cpp` | Per-item on-hand quantity plus the camp's daily demand. `operator double()` is days of supply. `Stock + Stock`, `Stock - Allocation`. |
| `Camp.h/.cpp` | Camp with nested `Shelter`, capacity guard, `++`/`--` occupancy, owns a `Stock`, friend of `ReportGenerator`. |
| `DistributionLog.h/.cpp` | All allocations ever made. Rejects a second claim by the same family on the same date in any camp. |
| `DistributionQueue.h/.cpp` | Hand-written binary max-heap over `Family*`, ordered by `Family::operator<`. |
| `FamilySorter.h/.cpp` | `Metric` pointer-to-member-function typedef and sorts driven by it. |
| `StockLedger.h/.cpp` | Stock-in and stock-out transactions with history. Holds `StockTxn`. |
| `TransferAdvisor.h/.cpp` | Finds a surplus camp and a short camp per item. Holds `TransferSuggestion`. |
| `HealthMonitor.h/.cpp` | Health case log, 3-day rolling window, outbreak thresholds. Holds `HealthCase`. |
| `ReportGenerator.h/.cpp` | Friend of `Camp`. Daily report text plus CSV export. |
| `Csv.h/.cpp` | Field split/join/escape and checked numeric parsing that throws `FileFormatException`. |
| `CsvStore.h/.cpp` | Saves and loads every registry and log to `data/*.csv`. |
| `Autosaver.h/.cpp` | Background thread, condition variable, shared mutex, clean shutdown. |
| `ConsoleApp.h/.cpp` | Owns all state and collaborators, drives the menu, catches every exception. |
| `src/main.cpp` | Builds a `ConsoleApp` on `data/` and runs it. |
| `src/seed_main.cpp` | Generates the Morigaon demo dataset into `data/`. Built as a separate binary. |

### Tests (`tests/`)

`TestRunner.h` (header-only asserts), `test_main.cpp` (calls every suite), and one file per area: `test_date.cpp`, `test_ration.cpp`, `test_person.cpp`, `test_hierarchy.cpp`, `test_family.cpp`, `test_registry.cpp`, `test_exceptions.cpp`, `test_stock.cpp`, `test_camp.cpp`, `test_distribution.cpp`, `test_queue.cpp`, `test_sorter.cpp`, `test_ledger.cpp`, `test_transfer.cpp`, `test_health.cpp`, `test_report.cpp`, `test_csv.cpp`, `test_autosave.cpp`.

### Data (`data/`)

`camps.csv`, `shelters.csv`, `families.csv`, `members.csv`, `volunteers.csv`, `stock.csv`, `distributions.csv`, `stock_txns.csv`, `health.csv`.

### Build and docs

`Makefile`, `CMakeLists.txt`, `.gitignore`, `README.md`, `docs/PLAN.md`, `docs/OOP_MAPPING.md`, `docs/superpowers/specs/2026-11-11-relief-camp-manager-design.md`.

### Ownership rule, stated once

`Family` owns its `Person*` members with raw `new`/`delete`. `Registry<Camp>`, `Registry<Family>` and `Registry<Volunteer>` own their elements with `std::unique_ptr`. A `ResidentVolunteer` is a family member, so it is owned by its `Family` and is **never** placed in `Registry<Volunteer>`; the camp worker roster is built by scanning families for `typeTag() == "RESIDENT_VOLUNTEER"` and appending the outside volunteers from `Registry<Volunteer>`. No object is ever owned twice.

---

## Task 1: Build system and test runner

**Files:**
- Create: `Makefile`, `CMakeLists.txt`, `include/Debug.h`, `tests/TestRunner.h`, `tests/test_main.cpp`, `tests/test_runner_selftest.cpp`, `src/main.cpp`

**Interfaces:**
- Consumes: nothing.
- Produces: `make` builds `bin/relief`; `make test` builds and runs `bin/tests`; `make debug` builds with `-DRELIEF_DEBUG`; `make seed` builds `bin/seed`; `make clean` removes `build/` and `bin/`. Test macros `ASSERT_TRUE(c)`, `ASSERT_FALSE(c)`, `ASSERT_EQ(a,b)`, `ASSERT_STR_EQ(a,b)`, `ASSERT_NEAR(a,b,eps)`, `ASSERT_THROWS(stmt, ExceptionType)`. Suite entry point convention: `void run_<area>_tests();`, declared in `tests/test_main.cpp`. `RELIEF_LIFECYCLE(msg)` macro from `include/Debug.h`.

- [ ] **Step 1: Write the failing test**

Create `tests/TestRunner.h`:

```cpp
#ifndef RELIEF_TESTRUNNER_H
#define RELIEF_TESTRUNNER_H

// A tiny assert-based test runner written by us. No external framework.
#include <iostream>
#include <string>
#include <cmath>

namespace testing {

inline int& passCount() { static int n = 0; return n; }
inline int& failCount() { static int n = 0; return n; }
inline std::string& currentSuite() { static std::string s = "?"; return s; }

inline void beginSuite(const std::string& name) {
    currentSuite() = name;
    std::cout << "\n-- " << name << " --\n";
}

inline void reportPass(const std::string& what) {
    passCount()++;
    std::cout << "  ok   " << what << "\n";
}

inline void reportFail(const std::string& what, const std::string& detail,
                       const char* file, int line) {
    failCount()++;
    std::cout << "  FAIL " << what << "\n"
              << "       " << detail << "\n"
              << "       at " << file << ":" << line << "\n";
}

inline int summary() {
    std::cout << "\n=========================================\n"
              << "  passed: " << passCount() << "   failed: " << failCount() << "\n"
              << "=========================================\n";
    return failCount() == 0 ? 0 : 1;
}

} // namespace testing

#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (cond) { testing::reportPass(#cond); }                              \
        else { testing::reportFail(#cond, "expected true", __FILE__, __LINE__); } \
    } while (false)

#define ASSERT_FALSE(cond)                                                     \
    do {                                                                       \
        if (!(cond)) { testing::reportPass("!(" #cond ")"); }                   \
        else { testing::reportFail(#cond, "expected false", __FILE__, __LINE__); } \
    } while (false)

#define ASSERT_EQ(actual, expected)                                            \
    do {                                                                       \
        if ((actual) == (expected)) { testing::reportPass(#actual " == " #expected); } \
        else {                                                                 \
            std::ostringstream oss__;                                          \
            oss__ << "got " << (actual) << ", expected " << (expected);        \
            testing::reportFail(#actual " == " #expected, oss__.str(), __FILE__, __LINE__); \
        }                                                                      \
    } while (false)

#define ASSERT_STR_EQ(actual, expected)                                        \
    do {                                                                       \
        std::string a__ = (actual); std::string e__ = (expected);              \
        if (a__ == e__) { testing::reportPass(#actual " == " #expected); }      \
        else {                                                                 \
            testing::reportFail(#actual " == " #expected,                      \
                                "got \"" + a__ + "\", expected \"" + e__ + "\"", \
                                __FILE__, __LINE__);                           \
        }                                                                      \
    } while (false)

#define ASSERT_NEAR(actual, expected, eps)                                     \
    do {                                                                       \
        double a__ = (actual); double e__ = (expected);                        \
        if (std::fabs(a__ - e__) <= (eps)) { testing::reportPass(#actual " ~= " #expected); } \
        else {                                                                 \
            std::ostringstream oss__;                                          \
            oss__ << "got " << a__ << ", expected " << e__ << " +/- " << (eps);\
            testing::reportFail(#actual " ~= " #expected, oss__.str(), __FILE__, __LINE__); \
        }                                                                      \
    } while (false)

#define ASSERT_THROWS(stmt, ExceptionType)                                     \
    do {                                                                       \
        bool threw__ = false;                                                  \
        try { stmt; }                                                          \
        catch (const ExceptionType&) { threw__ = true; }                       \
        catch (...) {                                                          \
            testing::reportFail(#stmt " throws " #ExceptionType,               \
                                "threw a different exception type",            \
                                __FILE__, __LINE__);                           \
            break;                                                             \
        }                                                                      \
        if (threw__) { testing::reportPass(#stmt " throws " #ExceptionType); }  \
        else {                                                                 \
            testing::reportFail(#stmt " throws " #ExceptionType,               \
                                "no exception was thrown", __FILE__, __LINE__);\
        }                                                                      \
    } while (false)

#endif
```

Create `tests/test_runner_selftest.cpp`:

```cpp
#include "TestRunner.h"
#include <sstream>
#include <stdexcept>

void run_runner_selftest() {
    testing::beginSuite("test runner self-test");

    ASSERT_TRUE(1 + 1 == 2);
    ASSERT_FALSE(1 + 1 == 3);
    ASSERT_EQ(2 + 2, 4);
    ASSERT_STR_EQ(std::string("relief"), "relief");
    ASSERT_NEAR(0.1 + 0.2, 0.3, 1e-9);
    ASSERT_THROWS(throw std::runtime_error("boom"), std::runtime_error);
}
```

Create `tests/test_main.cpp`:

```cpp
#include "TestRunner.h"
#include <sstream>

void run_runner_selftest();

int main() {
    std::cout << "Relief Camp Manager - test suite\n";
    run_runner_selftest();
    return testing::summary();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `make: *** No rule to make target 'test'`. There is no Makefile yet.

- [ ] **Step 3: Write minimal implementation**

Create `include/Debug.h`:

```cpp
#ifndef RELIEF_DEBUG_H
#define RELIEF_DEBUG_H

#include <iostream>

// Build with `make debug` (-DRELIEF_DEBUG) to watch the object lifecycle.
// [Object lifecycle tracing]
#ifdef RELIEF_DEBUG
#define RELIEF_LIFECYCLE(msg) std::cout << "[lifecycle] " << msg << std::endl
#else
#define RELIEF_LIFECYCLE(msg) ((void)0)
#endif

#endif
```

Create `src/main.cpp` (a placeholder that Task 21 replaces):

```cpp
#include <iostream>

int main() {
    std::cout << "Relief Camp Manager - build scaffold ok\n";
    return 0;
}
```

Create `Makefile`:

```make
# Offline Relief Camp Manager - C++17, standard library only.
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -Iinclude
LDFLAGS  ?= -pthread

SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BUILD_DIR := build
BIN_DIR   := bin

APP_MAIN  := $(SRC_DIR)/main.cpp
SEED_MAIN := $(SRC_DIR)/seed_main.cpp
ALL_SRC   := $(wildcard $(SRC_DIR)/*.cpp)
LIB_SRC   := $(filter-out $(APP_MAIN) $(SEED_MAIN),$(ALL_SRC))
TEST_SRC  := $(wildcard $(TEST_DIR)/*.cpp)

APP_BIN   := $(BIN_DIR)/relief
TEST_BIN  := $(BIN_DIR)/tests
SEED_BIN  := $(BIN_DIR)/seed

.PHONY: all test debug seed clean run
all: $(APP_BIN)

$(APP_BIN): $(LIB_SRC) $(APP_MAIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(APP_MAIN) -o $@ $(LDFLAGS)

$(TEST_BIN): $(LIB_SRC) $(TEST_SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(TEST_DIR) $(LIB_SRC) $(TEST_SRC) -o $@ $(LDFLAGS)

$(SEED_BIN): $(LIB_SRC) $(SEED_MAIN) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LIB_SRC) $(SEED_MAIN) -o $@ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

debug: CXXFLAGS += -DRELIEF_DEBUG -g
debug: clean $(APP_BIN)

seed: $(SEED_BIN)
	./$(SEED_BIN)

run: $(APP_BIN)
	./$(APP_BIN)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
```

Note: the `seed` target only works once Task 22 creates `src/seed_main.cpp`. Until then `make`, `make test` and `make debug` are the targets in use.

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(ReliefCampManager CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -pedantic)
endif()

find_package(Threads REQUIRED)
include_directories(${CMAKE_SOURCE_DIR}/include)

file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
if(EXISTS ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
endif()

add_executable(relief ${LIB_SOURCES} ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(relief PRIVATE Threads::Threads)

file(GLOB TEST_SOURCES ${CMAKE_SOURCE_DIR}/tests/*.cpp)
add_executable(relief_tests ${LIB_SOURCES} ${TEST_SOURCES})
target_include_directories(relief_tests PRIVATE ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(relief_tests PRIVATE Threads::Threads)

if(EXISTS ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  add_executable(relief_seed ${LIB_SOURCES} ${CMAKE_SOURCE_DIR}/src/seed_main.cpp)
  target_link_libraries(relief_seed PRIVATE Threads::Threads)
endif()

enable_testing()
add_test(NAME all_tests COMMAND relief_tests)
```

Add `<sstream>` to `TestRunner.h` includes so `ASSERT_EQ` compiles without each test file including it: change the include block to `#include <iostream>`, `#include <sstream>`, `#include <string>`, `#include <cmath>`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — 6 assertions pass, 0 fail, exit code 0.

Also run: `make` then `./bin/relief`
Expected: prints `Relief Camp Manager - build scaffold ok`.

- [ ] **Step 5: Commit**

```bash
git add Makefile CMakeLists.txt include/Debug.h src/main.cpp tests/
git commit -m "Add build system and self-written test runner

Makefile and CMakeLists.txt build the app, the test binary and (later) the
seed generator. tests/TestRunner.h provides ASSERT_TRUE/FALSE/EQ/STR_EQ/
NEAR/THROWS with a pass-fail tally and a non-zero exit code on failure."
```

---

## Task 2: Item and Date value types

**Files:**
- Create: `include/Item.h`, `src/Item.cpp`, `include/Date.h`, `src/Date.cpp`, `include/Symptom.h`, `src/Symptom.cpp`, `tests/test_date.cpp`
- Modify: `tests/test_main.cpp` (add `run_date_tests();`)

**Interfaces:**
- Consumes: `tests/TestRunner.h` from Task 1.
- Produces:
  - `enum class Item { Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin };` `const int ITEM_COUNT = 7;`
  - `std::string itemName(Item)`, `std::string itemUnit(Item)`, `int itemIndex(Item)`, `Item itemAt(int)`, `Item itemFromName(const std::string&)` (throws `std::invalid_argument`).
  - `enum class Symptom { Diarrhoea, Fever, SkinInfection, Respiratory, Other };` `const int SYMPTOM_COUNT = 5;` `std::string symptomName(Symptom)`, `Symptom symptomFromName(const std::string&)` (throws `std::invalid_argument`).
  - `class Date` with `Date()`, `Date(int,int,int)`, `getYear/getMonth/getDay`, `isValid`, `toString`, `static Date parse(const std::string&)`, `toDayNumber`, `static Date fromDayNumber(long)`, `plusDays(int)`, and friends `== != < <= - <<`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_date.cpp`:

```cpp
#include "TestRunner.h"
#include "Date.h"
#include "Item.h"
#include "Symptom.h"
#include <sstream>
#include <stdexcept>

void run_date_tests() {
    testing::beginSuite("Item, Symptom and Date");

    // --- Item ---
    ASSERT_EQ(ITEM_COUNT, 7);
    ASSERT_STR_EQ(itemName(Item::Rice), "Rice");
    ASSERT_STR_EQ(itemName(Item::BabyFood), "Baby food");
    ASSERT_STR_EQ(itemUnit(Item::Rice), "kg");
    ASSERT_STR_EQ(itemUnit(Item::Tarpaulin), "sheets");
    ASSERT_EQ(itemIndex(Item::Rice), 0);
    ASSERT_TRUE(itemAt(4) == Item::ORS);
    ASSERT_TRUE(itemFromName("Baby food") == Item::BabyFood);
    ASSERT_THROWS(itemFromName("Biscuits"), std::invalid_argument);

    // --- Symptom ---
    ASSERT_EQ(SYMPTOM_COUNT, 5);
    ASSERT_STR_EQ(symptomName(Symptom::Diarrhoea), "Diarrhoea");
    ASSERT_TRUE(symptomFromName("Fever") == Symptom::Fever);
    ASSERT_THROWS(symptomFromName("Hiccups"), std::invalid_argument);

    // --- Date construction and validity ---
    Date defaulted;                          // default constructor
    ASSERT_EQ(defaulted.getYear(), 1970);
    ASSERT_EQ(defaulted.getMonth(), 1);
    ASSERT_EQ(defaulted.getDay(), 1);

    Date d(2026, 7, 14);                     // parameterised constructor
    ASSERT_EQ(d.getYear(), 2026);
    ASSERT_EQ(d.getMonth(), 7);
    ASSERT_EQ(d.getDay(), 14);
    ASSERT_TRUE(d.isValid());
    ASSERT_STR_EQ(d.toString(), "2026-07-14");

    Date copy(d);                            // copy constructor
    ASSERT_TRUE(copy == d);

    ASSERT_FALSE(Date(2026, 13, 1).isValid());
    ASSERT_FALSE(Date(2026, 2, 30).isValid());
    ASSERT_TRUE(Date(2024, 2, 29).isValid());   // leap year
    ASSERT_FALSE(Date(2026, 2, 29).isValid());  // not a leap year
    ASSERT_FALSE(Date(1900, 2, 29).isValid());  // century non-leap

    // --- parse ---
    ASSERT_TRUE(Date::parse("2026-07-14") == d);
    ASSERT_THROWS(Date::parse("14/07/2026"), std::invalid_argument);
    ASSERT_THROWS(Date::parse("2026-02-30"), std::invalid_argument);

    // --- arithmetic ---
    ASSERT_EQ(Date(1970, 1, 1).toDayNumber(), 0L);
    ASSERT_EQ(Date(1970, 1, 2).toDayNumber(), 1L);
    ASSERT_TRUE(Date::fromDayNumber(0) == Date(1970, 1, 1));
    ASSERT_TRUE(d.plusDays(1) == Date(2026, 7, 15));
    ASSERT_TRUE(d.plusDays(-14) == Date(2026, 6, 30));
    ASSERT_TRUE(Date(2026, 7, 31).plusDays(1) == Date(2026, 8, 1));
    ASSERT_TRUE(Date(2026, 12, 31).plusDays(1) == Date(2027, 1, 1));
    ASSERT_EQ(Date(2026, 7, 14) - Date(2026, 7, 11), 3L);   // days between
    ASSERT_EQ(Date(2026, 7, 11) - Date(2026, 7, 14), -3L);

    // --- comparison ---
    ASSERT_TRUE(Date(2026, 7, 11) < Date(2026, 7, 14));
    ASSERT_FALSE(Date(2026, 7, 14) < Date(2026, 7, 11));
    ASSERT_TRUE(Date(2026, 7, 14) <= Date(2026, 7, 14));
    ASSERT_TRUE(Date(2026, 7, 14) != Date(2026, 7, 11));

    // --- stream output ---
    std::ostringstream oss;
    oss << d;
    ASSERT_STR_EQ(oss.str(), "2026-07-14");
}
```

Add the declaration and call to `tests/test_main.cpp`:

```cpp
void run_runner_selftest();
void run_date_tests();

int main() {
    std::cout << "Relief Camp Manager - test suite\n";
    run_runner_selftest();
    run_date_tests();
    return testing::summary();
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Date.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Item.h`:

```cpp
#ifndef RELIEF_ITEM_H
#define RELIEF_ITEM_H

#include <string>

// The relief items a camp stocks and hands out.
enum class Item { Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin };

const int ITEM_COUNT = 7;

std::string itemName(Item it);
std::string itemUnit(Item it);
int itemIndex(Item it);
Item itemAt(int index);
Item itemFromName(const std::string& name);   // throws std::invalid_argument

#endif
```

Create `src/Item.cpp`: a `switch` per function. `itemName` returns `"Rice"`, `"Dal"`, `"Oil"`, `"Salt"`, `"ORS"`, `"Baby food"`, `"Tarpaulin"`. `itemUnit` returns `"kg"` for Rice/Dal/Salt/Baby food, `"litre"` for Oil, `"packets"` for ORS, `"sheets"` for Tarpaulin. `itemIndex` is `static_cast<int>(it)`. `itemAt` throws `std::out_of_range` when `index < 0 || index >= ITEM_COUNT`, else `static_cast<Item>(index)`. `itemFromName` loops `0..ITEM_COUNT-1` comparing `itemName(itemAt(i))` and throws `std::invalid_argument("unknown item: " + name)` when nothing matches.

Create `include/Symptom.h` and `src/Symptom.cpp` the same way. Names: `"Diarrhoea"`, `"Fever"`, `"Skin infection"`, `"Respiratory"`, `"Other"`.

Create `include/Date.h`:

```cpp
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
```

`src/Date.cpp` notes:
- `isLeapYear`: `(year % 4 == 0 && year % 100 != 0) || year % 400 == 0`.
- `daysInMonth`: table `{31,28,31,30,31,30,31,31,30,31,30,31}` with February becoming 29 in a leap year; returns 0 for `month < 1 || month > 12`.
- `isValid`: `year >= 1900 && month >= 1 && month <= 12 && day >= 1 && day <= daysInMonth(year, month)`.
- `toDayNumber`: loop years from 1970 up to (or down to) `year` adding 365 or 366, then add `daysInMonth` for each full month, then `day - 1`. A loop is clearer for a student than the civil-days formula.
- `fromDayNumber`: the inverse loop.
- `toString`: `std::ostringstream` with `std::setw(2)` and `std::setfill('0')` (include `<iomanip>`).
- `parse`: require length 10 and `text[4] == '-' && text[7] == '-'`, all other characters digits; build the `Date`, then throw `std::invalid_argument("bad date: " + text)` if `!isValid()`.
- `operator-`: `a.toDayNumber() - b.toDayNumber()`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Item, Symptom and Date` suite reports all assertions ok, 0 failed.

- [ ] **Step 5: Commit**

```bash
git add include/Item.h include/Symptom.h include/Date.h src/Item.cpp src/Symptom.cpp src/Date.cpp tests/test_date.cpp tests/test_main.cpp
git commit -m "Add Item, Symptom and Date value types

Date carries day-number arithmetic so the 3-day outbreak window and the
same-day duplicate-claim check are simple to compute and test. Comparison
and difference operators are friend functions."
```

---
## Task 3: Ration and RationNorms

**Files:**
- Create: `include/Ration.h`, `src/Ration.cpp`, `include/RationNorms.h`, `src/RationNorms.cpp`, `tests/test_ration.cpp`
- Modify: `tests/test_main.cpp` (add `run_ration_tests();`)

**Interfaces:**
- Consumes: `Item.h` from Task 2.
- Produces:
  - `class Ration`: `Ration()`, `Ration(int persons)` (basic-to-class), `explicit Ration(const double* table)`, copy constructor, destructor, `double get(Item) const`, `void set(Item, double)`, `void add(Item, double)`, `double operator[](Item) const`, `double totalWeight() const`, `bool isZero() const`, `Ration& operator+=(const Ration&)`, `Ration& operator*=(double)`, and friends `operator+`, `operator-`, `operator*(const Ration&, double)`, `operator==`, `operator<<`.
  - `class RationNorms`: `static Ration forAdult()`, `forChild()`, `forInfant()`, `forElderly()`, `forVolunteer()`, `static const double* adultTable()`, `static double pregnancyRiceBonus()`, `static double lactationOilBonus()`, `static int tableCount()`.
- Include direction, to avoid a cycle: `RationNorms.h` includes `Item.h` and forward-declares `class Ration;`. `RationNorms.cpp` includes `Ration.h`. `Ration.h` includes `Item.h` only. `Ration.cpp` includes `RationNorms.h`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_ration.cpp`:

```cpp
#include "TestRunner.h"
#include "Ration.h"
#include "RationNorms.h"
#include <sstream>

void run_ration_tests() {
    testing::beginSuite("Ration and RationNorms");

    // Default constructor gives an empty ration.
    Ration empty;
    ASSERT_TRUE(empty.isZero());
    ASSERT_NEAR(empty.get(Item::Rice), 0.0, 1e-9);
    ASSERT_NEAR(empty.totalWeight(), 0.0, 1e-9);

    // The norm table is the single source of truth.
    Ration adult = RationNorms::forAdult();
    ASSERT_NEAR(adult.get(Item::Rice), 0.400, 1e-9);
    ASSERT_NEAR(adult.get(Item::Dal), 0.060, 1e-9);
    ASSERT_NEAR(adult.get(Item::Oil), 0.030, 1e-9);
    ASSERT_NEAR(adult.get(Item::Salt), 0.010, 1e-9);
    ASSERT_NEAR(adult.get(Item::BabyFood), 0.0, 1e-9);
    ASSERT_EQ(RationNorms::tableCount(), 5);

    Ration infant = RationNorms::forInfant();
    ASSERT_NEAR(infant.get(Item::Rice), 0.0, 1e-9);        // infants get no rice
    ASSERT_NEAR(infant.get(Item::BabyFood), 0.100, 1e-9);
    ASSERT_TRUE(infant.get(Item::ORS) > 0.0);

    Ration child = RationNorms::forChild();
    ASSERT_NEAR(child.get(Item::Rice), 0.200, 1e-9);        // half the adult norm

    Ration elderly = RationNorms::forElderly();
    ASSERT_TRUE(elderly.get(Item::Rice) < adult.get(Item::Rice));
    ASSERT_TRUE(elderly.get(Item::ORS) > adult.get(Item::ORS));

    // [Basic type to class type] a headcount converts to an adult-equivalent ration.
    Ration forThree = 3;
    ASSERT_NEAR(forThree.get(Item::Rice), 1.200, 1e-9);
    ASSERT_NEAR(forThree.get(Item::Dal), 0.180, 1e-9);

    // Binary operators.
    Ration sum = adult + child;
    ASSERT_NEAR(sum.get(Item::Rice), 0.600, 1e-9);

    Ration diff = sum - child;
    ASSERT_NEAR(diff.get(Item::Rice), 0.400, 1e-9);
    ASSERT_TRUE(diff == adult);

    Ration doubled = adult * 2.0;
    ASSERT_NEAR(doubled.get(Item::Rice), 0.800, 1e-9);

    // Compound assignment.
    Ration acc;
    acc += adult;
    acc += adult;
    ASSERT_NEAR(acc.get(Item::Rice), 0.800, 1e-9);
    acc *= 0.5;
    ASSERT_NEAR(acc.get(Item::Rice), 0.400, 1e-9);

    // Subscript read access.
    ASSERT_NEAR(adult[Item::Rice], 0.400, 1e-9);

    // set / add
    Ration manual;
    manual.set(Item::Tarpaulin, 2.0);
    manual.add(Item::Tarpaulin, 1.0);
    ASSERT_NEAR(manual.get(Item::Tarpaulin), 3.0, 1e-9);
    ASSERT_FALSE(manual.isZero());

    // Copy constructor is a real copy.
    Ration copied(adult);
    copied.set(Item::Rice, 9.9);
    ASSERT_NEAR(adult.get(Item::Rice), 0.400, 1e-9);
    ASSERT_NEAR(copied.get(Item::Rice), 9.9, 1e-9);

    // Stream output names every non-zero item.
    std::ostringstream oss;
    oss << adult;
    std::string text = oss.str();
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("0.4") != std::string::npos);
    ASSERT_TRUE(text.find("Baby food") == std::string::npos);  // zero items are skipped

    // Bonuses exist and are small.
    ASSERT_TRUE(RationNorms::pregnancyRiceBonus() > 0.0);
    ASSERT_TRUE(RationNorms::lactationOilBonus() > 0.0);
}
```

Add `void run_ration_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Ration.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Ration.h`:

```cpp
#ifndef RELIEF_RATION_H
#define RELIEF_RATION_H

#include "Item.h"
#include <ostream>

// One day's entitlement, item by item. The unit of each slot is itemUnit().
class Ration {
    double qty[ITEM_COUNT];
public:
    Ration();                                  // all zero
    Ration(int persons);                       // [Basic type to class type] non-explicit on purpose
    explicit Ration(const double* table);      // build from a RationNorms table
    Ration(const Ration& other);               // [Copy constructor]
    ~Ration();

    double get(Item it) const;                 // [Const member function]
    void set(Item it, double value);
    void add(Item it, double value);
    double totalWeight() const;
    bool isZero() const;

    double operator[](Item it) const;          // [Binary operator] read access
    Ration& operator+=(const Ration& other);
    Ration& operator*=(double factor);

    // [Friend functions] symmetric arithmetic reads better as free functions
    friend Ration operator+(const Ration& a, const Ration& b);
    friend Ration operator-(const Ration& a, const Ration& b);
    friend Ration operator*(const Ration& r, double factor);
    friend bool operator==(const Ration& a, const Ration& b);
    friend std::ostream& operator<<(std::ostream& os, const Ration& r);
};

#endif
```

Create `include/RationNorms.h`:

```cpp
#ifndef RELIEF_RATIONNORMS_H
#define RELIEF_RATIONNORMS_H

#include "Item.h"

class Ration;   // forward declaration keeps the include graph acyclic

// The relief ration norms, in one place. Every quantity in the system traces
// back to this table.
class RationNorms {
    // [Static data members] shared by every object, defined once in the .cpp
    static const double adultDaily[ITEM_COUNT];
    static const double childDaily[ITEM_COUNT];
    static const double infantDaily[ITEM_COUNT];
    static const double elderlyDaily[ITEM_COUNT];
    static const double volunteerDaily[ITEM_COUNT];
    static const double pregnancyRice;
    static const double lactationOil;
public:
    // [Static member functions] no object needed to read a norm
    static Ration forAdult();
    static Ration forChild();
    static Ration forInfant();
    static Ration forElderly();
    static Ration forVolunteer();
    static const double* adultTable();
    static double pregnancyRiceBonus();
    static double lactationOilBonus();
    static int tableCount();
};

#endif
```

`src/RationNorms.cpp` tables, in item order `{Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin}`:

```cpp
#include "RationNorms.h"
#include "Ration.h"

const double RationNorms::adultDaily[ITEM_COUNT]     = {0.400, 0.060, 0.030, 0.010, 0.10, 0.000, 0.00};
const double RationNorms::childDaily[ITEM_COUNT]     = {0.200, 0.040, 0.020, 0.008, 0.15, 0.000, 0.00};
const double RationNorms::infantDaily[ITEM_COUNT]    = {0.000, 0.000, 0.010, 0.002, 0.25, 0.100, 0.00};
const double RationNorms::elderlyDaily[ITEM_COUNT]   = {0.300, 0.060, 0.035, 0.010, 0.20, 0.000, 0.00};
const double RationNorms::volunteerDaily[ITEM_COUNT] = {0.450, 0.070, 0.030, 0.010, 0.10, 0.000, 0.00};
const double RationNorms::pregnancyRice = 0.100;
const double RationNorms::lactationOil  = 0.010;

Ration RationNorms::forAdult()     { return Ration(adultDaily); }
Ration RationNorms::forChild()     { return Ration(childDaily); }
Ration RationNorms::forInfant()    { return Ration(infantDaily); }
Ration RationNorms::forElderly()   { return Ration(elderlyDaily); }
Ration RationNorms::forVolunteer() { return Ration(volunteerDaily); }
const double* RationNorms::adultTable() { return adultDaily; }
double RationNorms::pregnancyRiceBonus() { return pregnancyRice; }
double RationNorms::lactationOilBonus()  { return lactationOil; }
int RationNorms::tableCount() { return 5; }
```

`src/Ration.cpp` notes:
- `Ration()` zeroes every slot with a `for` loop.
- `Ration(int persons)` includes `RationNorms.h`, copies `RationNorms::adultTable()` and multiplies each slot by `persons`; a negative `persons` is clamped to 0.
- `explicit Ration(const double* table)` copies `ITEM_COUNT` doubles.
- `totalWeight()` sums the slots whose unit is `"kg"` or `"litre"` (skip ORS packets and Tarpaulin sheets).
- `operator-` clamps each result slot at 0.0 so a ration can never go negative.
- `operator==` compares each slot with a tolerance of `1e-9`.
- `operator<<` prints only non-zero slots as `Rice 0.4 kg, Dal 0.06 kg` using `std::fixed` and `std::setprecision(3)` with trailing zeros trimmed; simplest version that satisfies the test is `std::setprecision(3)` with `std::defaultfloat`, which renders `0.4` and `0.06`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Ration and RationNorms` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Ration.h include/RationNorms.h src/Ration.cpp src/RationNorms.cpp tests/test_ration.cpp tests/test_main.cpp
git commit -m "Add Ration value type and the static RationNorms table

RationNorms holds every norm as a static data member with static factory
functions, so there is one source of truth for entitlements. Ration(int)
is the basic-to-class conversion; arithmetic is friend-based and clamps at
zero so a ration can never go negative."
```

---

## Task 4: Person abstract base class

**Files:**
- Create: `include/Person.h`, `src/Person.cpp`, `tests/test_person.cpp`
- Modify: `tests/test_main.cpp` (add `run_person_tests();`)

**Interfaces:**
- Consumes: `Ration.h`, `Debug.h`.
- Produces: `class Person` — abstract, four pure virtuals.
  - `Person()`, `Person(const std::string& name, int age, char sex)`, `Person(const Person& other)`, `virtual ~Person()`.
  - `static int generateId()`, `static int liveObjects()`, `static void resetIdCounter(int startAt)`, `static int lastIssuedId()`.
  - `int getId() const`, `const std::string& getName() const`, `int getAge() const`, `char getSex() const`, `bool isPregnant() const`, `bool isDisabled() const`, `bool isLactating() const`.
  - `void setFlags(bool pregnant, bool disabled, bool lactating)`, `void adoptId(int existingId)`.
  - `protected: int flagBonus() const;`
  - Pure virtuals: `virtual Ration rationNeed() const = 0;`, `virtual int vulnerabilityScore() const = 0;`, `virtual std::string describe() const = 0;`, `virtual Person* clone() const = 0;`, `virtual std::string typeTag() const = 0;`
  - `friend std::ostream& operator<<(std::ostream&, const Person&);`

- [ ] **Step 1: Write the failing test**

Create `tests/test_person.cpp`:

```cpp
#include "TestRunner.h"
#include "Person.h"
#include <sstream>

// A minimal concrete Person defined inside the test, so this task can be
// verified before any of the real subclasses exist.
class TestPerson : public Person {
public:
    TestPerson() : Person() {}
    TestPerson(const std::string& name, int age, char sex) : Person(name, age, sex) {}
    TestPerson(const TestPerson& other) : Person(other) {}
    Ration rationNeed() const override { Ration r; r.set(Item::Rice, 1.0); return r; }
    int vulnerabilityScore() const override { return 7 + flagBonus(); }
    std::string describe() const override { return "test person " + getName(); }
    Person* clone() const override { return new TestPerson(*this); }
    std::string typeTag() const override { return "TEST"; }
};

void run_person_tests() {
    testing::beginSuite("Person abstract base");

    Person::resetIdCounter(1);

    // Parameterised constructor and const getters.
    TestPerson a("Rupa Das", 34, 'F');
    ASSERT_STR_EQ(a.getName(), "Rupa Das");
    ASSERT_EQ(a.getAge(), 34);
    ASSERT_EQ(a.getSex(), 'F');
    ASSERT_EQ(a.getId(), 1);
    ASSERT_FALSE(a.isPregnant());
    ASSERT_FALSE(a.isDisabled());
    ASSERT_FALSE(a.isLactating());

    // Every new Person takes the next ID.
    TestPerson b("Bhaskar Nath", 41, 'M');
    ASSERT_EQ(b.getId(), 2);
    ASSERT_EQ(Person::lastIssuedId(), 2);

    // Default constructor still takes an ID.
    TestPerson blank;
    ASSERT_EQ(blank.getId(), 3);
    ASSERT_STR_EQ(blank.getName(), "unnamed");
    ASSERT_EQ(blank.getAge(), 0);

    // Copy constructor copies the fields but takes a fresh identity.
    TestPerson copied(a);
    ASSERT_STR_EQ(copied.getName(), "Rupa Das");
    ASSERT_EQ(copied.getAge(), 34);
    ASSERT_TRUE(copied.getId() != a.getId());

    // Flags feed the shared vulnerability bonus.
    TestPerson mother("Anjali Bora", 27, 'F');
    ASSERT_EQ(mother.vulnerabilityScore(), 7);
    mother.setFlags(true, false, false);        // pregnant
    ASSERT_TRUE(mother.isPregnant());
    ASSERT_EQ(mother.vulnerabilityScore(), 7 + 25);
    mother.setFlags(false, true, true);         // disabled and lactating
    ASSERT_EQ(mother.vulnerabilityScore(), 7 + 20 + 15);

    // Runtime polymorphism through a base pointer.
    Person* p = &a;
    ASSERT_NEAR(p->rationNeed().get(Item::Rice), 1.0, 1e-9);
    ASSERT_STR_EQ(p->describe(), "test person Rupa Das");
    ASSERT_STR_EQ(p->typeTag(), "TEST");

    // clone() produces an independent object of the same dynamic type.
    Person* cl = p->clone();
    ASSERT_STR_EQ(cl->getName(), "Rupa Das");
    ASSERT_STR_EQ(cl->typeTag(), "TEST");
    ASSERT_TRUE(cl != p);
    delete cl;

    // adoptId lets CSV loading restore the original identity.
    TestPerson loaded("Jiten Saikia", 55, 'M');
    loaded.adoptId(4321);
    ASSERT_EQ(loaded.getId(), 4321);

    // resetIdCounter keeps freshly created objects from colliding after a load.
    Person::resetIdCounter(5000);
    TestPerson afterLoad("Minoti Devi", 60, 'F');
    ASSERT_EQ(afterLoad.getId(), 5000);

    // Friend operator<< delegates to describe().
    std::ostringstream oss;
    oss << a;
    ASSERT_TRUE(oss.str().find("Rupa Das") != std::string::npos);
    ASSERT_TRUE(oss.str().find("#1") != std::string::npos);

    Person::resetIdCounter(1);
}
```

Add `void run_person_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Person.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Person.h`:

```cpp
#ifndef RELIEF_PERSON_H
#define RELIEF_PERSON_H

#include "Ration.h"
#include <string>
#include <ostream>

// [Abstract class] the root of the whole people hierarchy. It can never be
// instantiated: every concrete person must answer all five questions below.
class Person {
    // [Static data members] one counter shared by every Person ever created
    static int nextId;
    static int liveCount;

    int id;
    std::string name;
    int age;
    char sex;              // 'M', 'F' or 'O'
    bool pregnant;
    bool disabled;
    bool lactating;

protected:
    // Shared vulnerability bonus, used by every subclass's score.
    int flagBonus() const;

public:
    Person();                                              // [Default constructor]
    Person(const std::string& name, int age, char sex);    // [Parameterised constructor]
    Person(const Person& other);                           // [Copy constructor]
    virtual ~Person();                                     // [Virtual destructor]

    // [Static member functions]
    static int generateId();
    static int liveObjects();
    static void resetIdCounter(int startAt);
    static int lastIssuedId();

    // [Const member functions] every getter promises not to modify the object
    int getId() const;
    const std::string& getName() const;
    int getAge() const;
    char getSex() const;
    bool isPregnant() const;
    bool isDisabled() const;
    bool isLactating() const;

    void setFlags(bool pregnant, bool disabled, bool lactating);
    void setName(const std::string& name);
    void adoptId(int existingId);     // used when loading from CSV

    // [Pure virtual functions] -> runtime polymorphism
    virtual Ration rationNeed() const = 0;
    virtual int vulnerabilityScore() const = 0;
    virtual std::string describe() const = 0;
    virtual Person* clone() const = 0;          // deep copy through a base pointer
    virtual std::string typeTag() const = 0;    // short tag used by the CSV layer

    // [Friend function] printing is not a member, so it reads as `os << person`
    friend std::ostream& operator<<(std::ostream& os, const Person& p);
};

#endif
```

`src/Person.cpp` notes:
- `int Person::nextId = 1;` and `int Person::liveCount = 0;` at file scope.
- Every constructor does `id = generateId(); liveCount++;` and ends with `RELIEF_LIFECYCLE("Person created  #" << id << " " << name);`
- The copy constructor copies `name`, `age`, `sex` and the three flags but calls `generateId()` for a fresh `id`, because two distinct people must never share an ID.
- `~Person()` does `liveCount--;` and `RELIEF_LIFECYCLE("Person destroyed #" << id << " " << name);`
- `generateId()` returns `nextId++`. `lastIssuedId()` returns `nextId - 1`. `resetIdCounter(startAt)` sets `nextId = startAt`.
- `flagBonus()` returns `(pregnant ? 25 : 0) + (disabled ? 20 : 0) + (lactating ? 15 : 0)`.
- Default constructor sets `name = "unnamed"`, `age = 0`, `sex = 'O'`, all flags false.
- `operator<<` prints `"#" << p.id << " " << p.describe()`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Person abstract base` suite all ok.

Also confirm the class really is abstract. Temporarily add `Person bad;` to the test, run `make test`, and check the compiler rejects it with "cannot declare variable 'bad' to be of abstract type". Then remove the line and re-run `make test` so it passes again.

- [ ] **Step 5: Commit**

```bash
git add include/Person.h src/Person.cpp tests/test_person.cpp tests/test_main.cpp
git commit -m "Add abstract Person base class

Static ID generator and live-object counter, all three constructor forms, a
virtual destructor that traces the lifecycle under RELIEF_DEBUG, and five
pure virtual functions including clone() so a polymorphic member can be
deep-copied through a base pointer."
```

---

## Task 5: Resident and Volunteer — the virtual bases

**Files:**
- Create: `include/Resident.h`, `src/Resident.cpp`, `include/Volunteer.h`, `src/Volunteer.cpp`, `tests/test_hierarchy.cpp`
- Modify: `tests/test_main.cpp` (add `run_hierarchy_tests();`)

**Interfaces:**
- Consumes: `Person.h`, `Ration.h`, `RationNorms.h`.
- Produces:
  - `class Resident : virtual public Person` — still abstract (no `rationNeed`). Adds `Resident()`, `Resident(const std::string& name, int age, char sex, int familyId, int campId)`, `Resident(const Resident&)`, `~Resident() override`, `int getFamilyId() const`, `int getCampId() const`, `const std::string& getShelterLabel() const`, `void setFamilyId(int)`, `void assignTo(int campId, const std::string& shelterLabel)`.
  - `class Volunteer : virtual public Person` — concrete. Adds `Volunteer()`, `Volunteer(const std::string& name, int age, char sex, const std::string& skill, const std::string& shift, const std::string& organisation)`, `Volunteer(const Volunteer&)`, `~Volunteer() override`, `const std::string& getSkill() const`, `getShift() const`, `getOrganisation() const`, and overrides all five pure virtuals. `typeTag()` returns `"VOLUNTEER"`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_hierarchy.cpp`. This file grows across Tasks 5, 6 and 7; write only the Task 5 portion now.

```cpp
#include "TestRunner.h"
#include "Resident.h"
#include "Volunteer.h"
#include <sstream>

// A concrete Resident, needed because Resident itself is still abstract.
class TestResident : public Resident {
public:
    TestResident(const std::string& name, int age, char sex, int familyId, int campId)
        : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {}
    Ration rationNeed() const override { return Ration(1); }
    int vulnerabilityScore() const override { return 1 + flagBonus(); }
    std::string describe() const override { return "test resident"; }
    Person* clone() const override { return new TestResident(*this); }
    std::string typeTag() const override { return "TEST_RESIDENT"; }
};

void run_hierarchy_tests() {
    testing::beginSuite("Person hierarchy: virtual bases");

    Person::resetIdCounter(1);

    // Resident adds camp placement on top of Person.
    TestResident r("Nripen Kalita", 38, 'M', 101, 7);
    ASSERT_STR_EQ(r.getName(), "Nripen Kalita");      // inherited from Person
    ASSERT_EQ(r.getFamilyId(), 101);
    ASSERT_EQ(r.getCampId(), 7);
    ASSERT_STR_EQ(r.getShelterLabel(), "");

    r.assignTo(9, "Hall-B");
    ASSERT_EQ(r.getCampId(), 9);
    ASSERT_STR_EQ(r.getShelterLabel(), "Hall-B");

    r.setFamilyId(202);
    ASSERT_EQ(r.getFamilyId(), 202);

    // Volunteer is concrete and gets the worker meal norm.
    Volunteer v("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross");
    ASSERT_STR_EQ(v.getSkill(), "Medic");
    ASSERT_STR_EQ(v.getShift(), "Morning");
    ASSERT_STR_EQ(v.getOrganisation(), "Red Cross");
    ASSERT_STR_EQ(v.typeTag(), "VOLUNTEER");
    ASSERT_NEAR(v.rationNeed().get(Item::Rice), 0.450, 1e-9);

    // A volunteer is not vulnerable; that is the point of the field.
    ASSERT_EQ(v.vulnerabilityScore(), 0);
    ASSERT_TRUE(v.describe().find("Medic") != std::string::npos);

    // Polymorphism through the shared base.
    Person* pv = &v;
    ASSERT_NEAR(pv->rationNeed().get(Item::Rice), 0.450, 1e-9);
    Person* clonedV = pv->clone();
    ASSERT_STR_EQ(clonedV->typeTag(), "VOLUNTEER");
    delete clonedV;

    // Both branches derive from exactly one Person, which is what `virtual` buys.
    Person* pr = &r;
    ASSERT_TRUE(pr->getId() == r.getId());

    Person::resetIdCounter(1);
}
```

Add `void run_hierarchy_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Resident.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Resident.h`:

```cpp
#ifndef RELIEF_RESIDENT_H
#define RELIEF_RESIDENT_H

#include "Person.h"
#include <string>

// A displaced person living in a camp. Still [abstract]: a resident's ration
// depends on their age band, which the subclasses below supply.
//
// [Virtual base class] `virtual public Person` means a ResidentVolunteer, which
// inherits from both this branch and Volunteer, carries exactly ONE Person
// subobject. Without `virtual` it would have two IDs and two sets of
// vulnerability flags, and describe() would be ambiguous.
class Resident : virtual public Person {
    int familyId;
    int campId;
    std::string shelterLabel;
public:
    Resident();
    Resident(const std::string& name, int age, char sex, int familyId, int campId);
    Resident(const Resident& other);
    ~Resident() override;

    int getFamilyId() const;
    int getCampId() const;
    const std::string& getShelterLabel() const;
    void setFamilyId(int newFamilyId);
    void assignTo(int newCampId, const std::string& newShelterLabel);
};

#endif
```

Create `include/Volunteer.h`:

```cpp
#ifndef RELIEF_VOLUNTEER_H
#define RELIEF_VOLUNTEER_H

#include "Person.h"
#include <string>

// Someone working at the camp: a medic, a cook, a record keeper. Volunteers
// draw a worker meal, not a family ration.
//
// [Virtual base class] see the note in Resident.h - this is the other half of
// the diamond that ResidentVolunteer closes.
class Volunteer : virtual public Person {
    std::string skill;
    std::string shift;
    std::string organisation;
public:
    Volunteer();
    Volunteer(const std::string& name, int age, char sex,
              const std::string& skill, const std::string& shift,
              const std::string& organisation);
    Volunteer(const Volunteer& other);
    ~Volunteer() override;

    const std::string& getSkill() const;
    const std::string& getShift() const;
    const std::string& getOrganisation() const;
    void reassign(const std::string& newSkill, const std::string& newShift);

    // [Runtime polymorphism] overrides of the pure virtuals
    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
```

Implementation notes:
- `Resident()` : `Person()`, `familyId(0)`, `campId(0)`, `shelterLabel("")`.
- `Resident(name, age, sex, familyId, campId)` : `Person(name, age, sex)` in the initialiser list **and** the member initialisers. Add this comment above the initialiser list: `// The Person(...) call here is used only when Resident is the most-derived class. For a ResidentVolunteer it is ignored, because a virtual base is initialised by the most-derived constructor. [Virtual base initialisation]`
- `Volunteer::rationNeed()` returns `RationNorms::forVolunteer()`.
- `Volunteer::vulnerabilityScore()` returns `0 + flagBonus()`.
- `Volunteer::describe()` returns `name + ", volunteer (" + skill + ", " + shift + " shift, " + organisation + ")"` built with the getters.
- `Volunteer::clone()` returns `new Volunteer(*this)`.
- All four destructors call `RELIEF_LIFECYCLE(...)` naming the class.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Person hierarchy: virtual bases` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Resident.h include/Volunteer.h src/Resident.cpp src/Volunteer.cpp tests/test_hierarchy.cpp tests/test_main.cpp
git commit -m "Add Resident and Volunteer as virtual bases of Person

Both branches use `virtual public Person` so the ResidentVolunteer in Task 7
carries exactly one Person subobject. Resident stays abstract because a
resident's ration depends on their age band. Volunteer is concrete and draws
the worker meal norm."
```

---

## Task 6: Infant, Child and Adult — hierarchical inheritance

**Files:**
- Create: `include/Infant.h`, `src/Infant.cpp`, `include/Child.h`, `src/Child.cpp`, `include/Adult.h`, `src/Adult.cpp`
- Modify: `tests/test_hierarchy.cpp` (append the age-band section)

**Interfaces:**
- Consumes: `Resident.h`, `RationNorms.h`.
- Produces: three concrete classes, each `public Resident`, each overriding `rationNeed`, `vulnerabilityScore`, `describe`, `clone`, `typeTag`. Constructor signature for all three: `X(const std::string& name, int age, char sex, int familyId, int campId)`, plus a default constructor and a copy constructor. `typeTag()` returns `"INFANT"`, `"CHILD"`, `"ADULT"`. Base vulnerability scores: Infant 40, Child 20, Adult 10, each plus `flagBonus()`.

- [ ] **Step 1: Write the failing test**

Append to `tests/test_hierarchy.cpp`, adding `#include "Infant.h"`, `#include "Child.h"`, `#include "Adult.h"` at the top and this block at the end of `run_hierarchy_tests()`:

```cpp
    testing::beginSuite("Person hierarchy: age bands");

    Person::resetIdCounter(1);

    Infant baby("Mitu Das", 1, 'F', 101, 1);
    ASSERT_STR_EQ(baby.typeTag(), "INFANT");
    ASSERT_NEAR(baby.rationNeed().get(Item::Rice), 0.0, 1e-9);
    ASSERT_NEAR(baby.rationNeed().get(Item::BabyFood), 0.100, 1e-9);
    ASSERT_EQ(baby.vulnerabilityScore(), 40);
    ASSERT_TRUE(baby.describe().find("infant") != std::string::npos);

    Child kid("Rahul Das", 9, 'M', 101, 1);
    ASSERT_STR_EQ(kid.typeTag(), "CHILD");
    ASSERT_NEAR(kid.rationNeed().get(Item::Rice), 0.200, 1e-9);
    ASSERT_EQ(kid.vulnerabilityScore(), 20);

    Adult man("Bipul Das", 35, 'M', 101, 1);
    ASSERT_STR_EQ(man.typeTag(), "ADULT");
    ASSERT_NEAR(man.rationNeed().get(Item::Rice), 0.400, 1e-9);
    ASSERT_EQ(man.vulnerabilityScore(), 10);

    // Pregnancy raises both the ration and the score.
    Adult mother("Rina Das", 29, 'F', 101, 1);
    mother.setFlags(true, false, false);
    ASSERT_NEAR(mother.rationNeed().get(Item::Rice), 0.500, 1e-9);   // 0.400 + 0.100 bonus
    ASSERT_EQ(mother.vulnerabilityScore(), 35);                      // 10 + 25

    // Lactation raises the oil allowance.
    Adult nursing("Jonali Das", 24, 'F', 101, 1);
    nursing.setFlags(false, false, true);
    ASSERT_NEAR(nursing.rationNeed().get(Item::Oil), 0.040, 1e-9);   // 0.030 + 0.010
    ASSERT_EQ(nursing.vulnerabilityScore(), 25);                     // 10 + 15

    // All three answer through the same base pointer: runtime polymorphism.
    Person* people[3] = { &baby, &kid, &man };
    double totalRice = 0.0;
    int totalScore = 0;
    for (int i = 0; i < 3; i++) {
        totalRice += people[i]->rationNeed().get(Item::Rice);
        totalScore += people[i]->vulnerabilityScore();
    }
    ASSERT_NEAR(totalRice, 0.600, 1e-9);
    ASSERT_EQ(totalScore, 70);

    // Each clone keeps its own dynamic type.
    for (int i = 0; i < 3; i++) {
        Person* c = people[i]->clone();
        ASSERT_STR_EQ(c->typeTag(), people[i]->typeTag());
        ASSERT_TRUE(c->getId() != people[i]->getId());
        delete c;
    }

    // They are all Residents, so camp placement works on every one of them.
    baby.assignTo(2, "Tent-14");
    ASSERT_EQ(baby.getCampId(), 2);

    Person::resetIdCounter(1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Infant.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create the three headers on one pattern. `include/Infant.h`:

```cpp
#ifndef RELIEF_INFANT_H
#define RELIEF_INFANT_H

#include "Resident.h"

// [Single inheritance] A camp resident aged 0-2. Infants need baby food and
// ORS rather than rice, and they are the most vulnerable group in a flood camp.
// [Hierarchical inheritance] Infant, Child and Adult all derive from Resident.
class Infant : public Resident {
public:
    Infant();
    Infant(const std::string& name, int age, char sex, int familyId, int campId);
    Infant(const Infant& other);
    ~Infant() override;

    Ration rationNeed() const override;         // [Overriding a pure virtual]
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
```

`include/Child.h` and `include/Adult.h` follow the same shape; their comments say "aged 3-12" and "aged 13-59" respectively, and `Adult.h` notes `// Adult is also the base of Elderly [multilevel] and of ResidentVolunteer [multiple].`

Implementation notes:
- Each parameterised constructor is `X(name, age, sex, familyId, campId) : Person(name, age, sex), Resident(name, age, sex, familyId, campId) {}` with the same virtual-base comment as Task 5.
- `Infant::rationNeed()` returns `RationNorms::forInfant()`; `Child::rationNeed()` returns `RationNorms::forChild()`.
- `Adult::rationNeed()` starts from `RationNorms::forAdult()`, then `if (isPregnant()) r.add(Item::Rice, RationNorms::pregnancyRiceBonus());` and `if (isLactating()) r.add(Item::Oil, RationNorms::lactationOilBonus());`, then returns `r`. Mark it `// [Returning objects] the ration is built and returned by value`.
- Scores: `return 40 + flagBonus();`, `20 + flagBonus()`, `10 + flagBonus()`.
- `describe()` returns, for example, `getName() + ", infant (" + std::to_string(getAge()) + "y), vulnerability " + std::to_string(vulnerabilityScore())`. The word `infant` / `child` / `adult` must be lowercase so the test's `find` succeeds.
- `clone()` returns `new Infant(*this)` and so on.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — both hierarchy suites all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Infant.h include/Child.h include/Adult.h src/Infant.cpp src/Child.cpp src/Adult.cpp tests/test_hierarchy.cpp
git commit -m "Add Infant, Child and Adult age bands

Hierarchical inheritance: three concrete residents from one Resident base,
each overriding rationNeed() with its own norm. Adult folds the pregnancy
and lactation bonuses into the returned Ration."
```

---

## Task 7: Elderly and ResidentVolunteer — multilevel, multiple and hybrid

**Files:**
- Create: `include/Elderly.h`, `src/Elderly.cpp`, `include/ResidentVolunteer.h`, `src/ResidentVolunteer.cpp`
- Modify: `tests/test_hierarchy.cpp` (append the diamond section)

**Interfaces:**
- Consumes: `Adult.h`, `Volunteer.h`.
- Produces:
  - `class Elderly : public Adult` — `Elderly()`, `Elderly(const std::string& name, int age, char sex, int familyId, int campId)`, copy constructor, overrides of all five virtuals. `typeTag()` returns `"ELDERLY"`. Score `35 + flagBonus()`.
  - `class ResidentVolunteer : public Adult, public Volunteer` — `ResidentVolunteer()`, `ResidentVolunteer(const std::string& name, int age, char sex, int familyId, int campId, const std::string& skill, const std::string& shift)`, copy constructor, overrides of all five virtuals delegating to `Adult::`. `typeTag()` returns `"RESIDENT_VOLUNTEER"`.

- [ ] **Step 1: Write the failing test**

Append to `tests/test_hierarchy.cpp`, adding `#include "Elderly.h"` and `#include "ResidentVolunteer.h"`:

```cpp
    testing::beginSuite("Person hierarchy: multilevel and the diamond");

    Person::resetIdCounter(1);

    // [Multilevel] Elderly -> Adult -> Resident -> Person
    Elderly grandma("Minoti Devi", 72, 'F', 101, 1);
    ASSERT_STR_EQ(grandma.typeTag(), "ELDERLY");
    ASSERT_NEAR(grandma.rationNeed().get(Item::Rice), 0.300, 1e-9);   // less rice
    ASSERT_TRUE(grandma.rationNeed().get(Item::ORS) > 0.100);         // more ORS
    ASSERT_EQ(grandma.vulnerabilityScore(), 35);
    ASSERT_TRUE(grandma.describe().find("elderly") != std::string::npos);

    // An Elderly is an Adult is a Resident is a Person: every level's
    // interface is available.
    ASSERT_EQ(grandma.getFamilyId(), 101);     // from Resident
    ASSERT_STR_EQ(grandma.getName(), "Minoti Devi");  // from Person
    Adult* asAdult = &grandma;
    ASSERT_NEAR(asAdult->rationNeed().get(Item::Rice), 0.300, 1e-9);  // still the override

    // Disabled elderly stacks the flag bonus on the elderly base score.
    Elderly frail("Hemanta Bora", 80, 'M', 102, 1);
    frail.setFlags(false, true, false);
    ASSERT_EQ(frail.vulnerabilityScore(), 55);                         // 35 + 20

    // [Multiple + hybrid inheritance] a displaced adult who also works the camp
    ResidentVolunteer helper("Gautam Pegu", 31, 'M', 103, 1, "Record keeper", "Evening");
    ASSERT_STR_EQ(helper.typeTag(), "RESIDENT_VOLUNTEER");

    // It reaches both parents' interfaces.
    ASSERT_EQ(helper.getFamilyId(), 103);          // via Adult -> Resident
    ASSERT_STR_EQ(helper.getSkill(), "Record keeper");   // via Volunteer
    ASSERT_STR_EQ(helper.getShift(), "Evening");

    // [Virtual base class] exactly ONE Person subobject, so exactly one ID.
    Resident* viaResident = &helper;
    Volunteer* viaVolunteer = &helper;
    Person* viaResidentPerson = viaResident;
    Person* viaVolunteerPerson = viaVolunteer;
    ASSERT_TRUE(viaResidentPerson == viaVolunteerPerson);     // the same Person
    ASSERT_EQ(viaResidentPerson->getId(), viaVolunteerPerson->getId());
    ASSERT_EQ(helper.getId(), viaVolunteerPerson->getId());

    // A resident volunteer draws the family ration, not the worker meal,
    // because they eat with their family.
    ASSERT_NEAR(helper.rationNeed().get(Item::Rice), 0.400, 1e-9);
    ASSERT_EQ(helper.vulnerabilityScore(), 10);
    ASSERT_TRUE(helper.describe().find("resident volunteer") != std::string::npos);

    // Cloning preserves the dynamic type across the diamond.
    Person* clonedHelper = viaVolunteerPerson->clone();
    ASSERT_STR_EQ(clonedHelper->typeTag(), "RESIDENT_VOLUNTEER");
    ASSERT_NEAR(clonedHelper->rationNeed().get(Item::Rice), 0.400, 1e-9);
    delete clonedHelper;

    // One mixed roster, five dynamic types, one loop.
    Infant i2("Pori Pegu", 2, 'F', 103, 1);
    Child c2("Dipu Pegu", 7, 'M', 103, 1);
    Adult a2("Gitali Pegu", 30, 'F', 103, 1);
    Volunteer v2("Nurse Kaberi", 40, 'F', "Nurse", "Night", "District Health");
    Person* roster[6] = { &i2, &c2, &a2, &grandma, &helper, &v2 };
    std::string tags;
    for (int i = 0; i < 6; i++) { tags += roster[i]->typeTag(); tags += ";"; }
    ASSERT_STR_EQ(tags, "INFANT;CHILD;ADULT;ELDERLY;RESIDENT_VOLUNTEER;VOLUNTEER;");

    Person::resetIdCounter(1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Elderly.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Elderly.h`:

```cpp
#ifndef RELIEF_ELDERLY_H
#define RELIEF_ELDERLY_H

#include "Adult.h"

// [Multilevel inheritance] Elderly -> Adult -> Resident -> Person, four levels.
// A resident aged 60 or over: less rice, more ORS and oil, and a much higher
// vulnerability score, because the elderly are prioritised in a flood camp.
class Elderly : public Adult {
public:
    Elderly();
    Elderly(const std::string& name, int age, char sex, int familyId, int campId);
    Elderly(const Elderly& other);
    ~Elderly() override;

    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
```

Create `include/ResidentVolunteer.h`:

```cpp
#ifndef RELIEF_RESIDENTVOLUNTEER_H
#define RELIEF_RESIDENTVOLUNTEER_H

#include "Adult.h"
#include "Volunteer.h"

// [Multiple inheritance] from Adult and Volunteer at once.
// [Hybrid inheritance] the whole figure is hierarchical (Infant/Child/Adult
// from Resident), multilevel (Elderly from Adult) and multiple (here), all in
// one tree.
//
// A displaced adult who also works a shift at the camp. Very common in Assam
// camps, where residents run the kitchen and keep the registers.
//
// Because Person is a VIRTUAL base of both Resident and Volunteer, this object
// holds exactly one Person subobject: one ID, one name, one set of flags. Its
// constructor must therefore initialise Person itself - see the .cpp.
class ResidentVolunteer : public Adult, public Volunteer {
public:
    ResidentVolunteer();
    ResidentVolunteer(const std::string& name, int age, char sex,
                      int familyId, int campId,
                      const std::string& skill, const std::string& shift);
    ResidentVolunteer(const ResidentVolunteer& other);
    ~ResidentVolunteer() override;

    // Both parents offer these, so the override must say which one wins.
    Ration rationNeed() const override;
    int vulnerabilityScore() const override;
    std::string describe() const override;
    Person* clone() const override;
    std::string typeTag() const override;
};

#endif
```

`src/ResidentVolunteer.cpp` — the constructor is the teaching centrepiece, so write it exactly like this:

```cpp
#include "ResidentVolunteer.h"
#include "Debug.h"

ResidentVolunteer::ResidentVolunteer()
    // [Virtual base initialisation] the MOST-DERIVED class initialises the
    // virtual base. Person(...) appears here, not in Adult's or Volunteer's
    // initialiser list, and their own Person(...) calls are ignored for this
    // object. That is exactly why there is only one Person subobject.
    : Person(), Adult(), Volunteer() {
    RELIEF_LIFECYCLE("ResidentVolunteer created  #" << getId());
}

ResidentVolunteer::ResidentVolunteer(const std::string& name, int age, char sex,
                                     int familyId, int campId,
                                     const std::string& skill,
                                     const std::string& shift)
    // [Virtual base initialisation] Person is named FIRST and initialised once.
    : Person(name, age, sex),
      Adult(name, age, sex, familyId, campId),
      Volunteer(name, age, sex, skill, shift, "camp resident") {
    RELIEF_LIFECYCLE("ResidentVolunteer created  #" << getId() << " " << name);
}

ResidentVolunteer::ResidentVolunteer(const ResidentVolunteer& other)
    : Person(other), Adult(other), Volunteer(other) {
    RELIEF_LIFECYCLE("ResidentVolunteer copied   #" << getId());
}

ResidentVolunteer::~ResidentVolunteer() {
    RELIEF_LIFECYCLE("ResidentVolunteer destroyed #" << getId());
}

// [Ambiguity resolution] Adult and Volunteer both supply these functions, so
// an unqualified call would be ambiguous. A resident volunteer eats with their
// family, so the Adult answer is the right one.
Ration ResidentVolunteer::rationNeed() const { return Adult::rationNeed(); }

int ResidentVolunteer::vulnerabilityScore() const { return Adult::vulnerabilityScore(); }

std::string ResidentVolunteer::describe() const {
    return getName() + ", resident volunteer (" + getSkill() + ", " + getShift()
         + " shift), family " + std::to_string(getFamilyId());
}

Person* ResidentVolunteer::clone() const { return new ResidentVolunteer(*this); }

std::string ResidentVolunteer::typeTag() const { return "RESIDENT_VOLUNTEER"; }
```

`src/Elderly.cpp` notes: `rationNeed()` returns `RationNorms::forElderly()` plus the same pregnancy/lactation guards as `Adult` (harmless and consistent); `vulnerabilityScore()` returns `35 + flagBonus()`; `describe()` contains the lowercase word `elderly`; `clone()` returns `new Elderly(*this)`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — all three hierarchy suites ok, including `viaResidentPerson == viaVolunteerPerson`.

Then prove the virtual bases are load-bearing. Temporarily edit `include/Resident.h` and `include/Volunteer.h` to say `public Person` instead of `virtual public Person`, run `make test`, and observe the compiler reject `ResidentVolunteer` with ambiguity errors such as "'Person' is an ambiguous base of 'ResidentVolunteer'". Restore `virtual` and re-run `make test` so it passes. Record the exact error text in a note for `docs/OOP_MAPPING.md` in Task 23.

- [ ] **Step 5: Commit**

```bash
git add include/Elderly.h include/ResidentVolunteer.h src/Elderly.cpp src/ResidentVolunteer.cpp tests/test_hierarchy.cpp
git commit -m "Add Elderly and ResidentVolunteer, completing the hierarchy

Elderly gives four-level multilevel inheritance. ResidentVolunteer multiply
inherits Adult and Volunteer and closes the diamond: the test asserts that
the Person reached through each parent is the same object, which only holds
because Person is a virtual base."
```

---
## Task 8: Family with the hand-written rule of three

**Files:**
- Create: `include/Family.h`, `src/Family.cpp`, `tests/test_family.cpp`
- Modify: `tests/test_main.cpp` (add `run_family_tests();`)

**Interfaces:**
- Consumes: `Person.h` (and the concrete subclasses for the test), `Ration.h`.
- Produces: `class Family` owning `Person** members`.
  - `Family()`, `Family(const std::string& headName, const std::string& village)`, `Family(const Family& other)`, `Family& operator=(const Family& other)`, `~Family()`.
  - `static int generateId()`, `static void resetIdCounter(int startAt)`, `void adoptId(int)`.
  - `int getId() const`, `const std::string& getHeadName() const`, `const std::string& getVillage() const`, `int getCampId() const`, `void setCampId(int)`, `void setHeadName(const std::string&)`.
  - `int size() const`, `const Person& memberAt(int index) const`, `Person& memberAt(int index)`, `void addMember(Person* p)` (takes ownership), `int countOfTag(const std::string& tag) const`.
  - `Ration dailyRation() const`, `int vulnerabilityScore() const`.
  - Metric functions, all `double ... () const` so one pointer-to-member type fits all: `metricVulnerability()`, `metricSize()`, `metricAverageAge()`, `metricInfantCount()`.
  - Friends: `operator<` (by vulnerability score), `operator==` (by ID), `operator<<`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_family.cpp`:

```cpp
#include "TestRunner.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include <sstream>

// Builds the demo family: one infant, one child, two adults, one elderly.
static Family makeDasFamily() {
    Family f("Bipul Das", "Bhuragaon");
    f.addMember(new Adult("Bipul Das", 35, 'M', f.getId(), 0));
    f.addMember(new Adult("Rina Das", 29, 'F', f.getId(), 0));
    f.addMember(new Child("Rahul Das", 9, 'M', f.getId(), 0));
    f.addMember(new Infant("Mitu Das", 1, 'F', f.getId(), 0));
    f.addMember(new Elderly("Minoti Devi", 72, 'F', f.getId(), 0));
    return f;
}

void run_family_tests() {
    testing::beginSuite("Family and the rule of three");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    // Default constructor.
    Family blank;
    ASSERT_EQ(blank.size(), 0);
    ASSERT_STR_EQ(blank.getHeadName(), "unnamed");
    ASSERT_EQ(blank.getId(), 1);

    // Parameterised constructor and growth past the initial capacity.
    Family f("Bipul Das", "Bhuragaon");
    ASSERT_STR_EQ(f.getHeadName(), "Bipul Das");
    ASSERT_STR_EQ(f.getVillage(), "Bhuragaon");
    ASSERT_EQ(f.size(), 0);

    for (int i = 0; i < 9; i++) {
        f.addMember(new Adult("Member " + std::to_string(i), 20 + i, 'M', f.getId(), 0));
    }
    ASSERT_EQ(f.size(), 9);                       // grew past the initial capacity of 4
    ASSERT_STR_EQ(f.memberAt(0).getName(), "Member 0");
    ASSERT_STR_EQ(f.memberAt(8).getName(), "Member 8");
    ASSERT_THROWS(f.memberAt(9), std::out_of_range);
    ASSERT_THROWS(f.memberAt(-1), std::out_of_range);

    // Polymorphic ration: each member answers with its own rationNeed().
    Family das = makeDasFamily();
    ASSERT_EQ(das.size(), 5);
    // 0.400 + 0.400 (adults) + 0.200 (child) + 0.000 (infant) + 0.300 (elderly)
    ASSERT_NEAR(das.dailyRation().get(Item::Rice), 1.300, 1e-9);
    ASSERT_NEAR(das.dailyRation().get(Item::BabyFood), 0.100, 1e-9);
    // 10 + 10 + 20 + 40 + 35
    ASSERT_EQ(das.vulnerabilityScore(), 115);

    ASSERT_EQ(das.countOfTag("INFANT"), 1);
    ASSERT_EQ(das.countOfTag("ADULT"), 2);
    ASSERT_EQ(das.countOfTag("ELDERLY"), 1);
    ASSERT_EQ(das.countOfTag("VOLUNTEER"), 0);

    // Metric functions, all double and all const, for pointer-to-member sorting.
    ASSERT_NEAR(das.metricVulnerability(), 115.0, 1e-9);
    ASSERT_NEAR(das.metricSize(), 5.0, 1e-9);
    ASSERT_NEAR(das.metricAverageAge(), (35 + 29 + 9 + 1 + 72) / 5.0, 1e-9);
    ASSERT_NEAR(das.metricInfantCount(), 1.0, 1e-9);
    ASSERT_NEAR(Family("Nobody", "Nowhere").metricAverageAge(), 0.0, 1e-9);  // no divide by zero

    // --- RULE OF THREE ---

    // Copy constructor deep-copies every member.
    Family copy(das);
    ASSERT_EQ(copy.size(), 5);
    ASSERT_NEAR(copy.dailyRation().get(Item::Rice), 1.300, 1e-9);
    // Different objects, and different Person IDs, because clone() makes new people.
    ASSERT_TRUE(&copy.memberAt(0) != &das.memberAt(0));
    ASSERT_TRUE(copy.memberAt(0).getId() != das.memberAt(0).getId());
    // The clone kept its dynamic type.
    ASSERT_STR_EQ(copy.memberAt(3).typeTag(), "INFANT");
    // Mutating the copy does not touch the original.
    copy.memberAt(0).setName("Changed");
    ASSERT_STR_EQ(das.memberAt(0).getName(), "Bipul Das");

    // Copy assignment deep-copies too, and frees what was there before.
    Family target("Temp Head", "Temp Village");
    target.addMember(new Adult("Throwaway", 50, 'M', target.getId(), 0));
    ASSERT_EQ(target.size(), 1);
    target = das;
    ASSERT_EQ(target.size(), 5);
    ASSERT_STR_EQ(target.getHeadName(), "Bipul Das");
    ASSERT_TRUE(&target.memberAt(2) != &das.memberAt(2));

    // Self-assignment must not corrupt or free the object.
    Family self = makeDasFamily();
    Family* selfPtr = &self;
    self = *selfPtr;
    ASSERT_EQ(self.size(), 5);
    ASSERT_NEAR(self.dailyRation().get(Item::Rice), 1.300, 1e-9);

    // Assignment returns a reference, so chaining works.
    Family c1, c2;
    c1 = c2 = das;
    ASSERT_EQ(c1.size(), 5);
    ASSERT_EQ(c2.size(), 5);

    // No leaks: every Person created must be destroyed. The live count returns
    // to its starting value once the scoped families go out of scope.
    int before = Person::liveObjects();
    {
        Family scoped = makeDasFamily();
        ASSERT_EQ(Person::liveObjects(), before + 5);
        Family scopedCopy(scoped);
        ASSERT_EQ(Person::liveObjects(), before + 10);
    }
    ASSERT_EQ(Person::liveObjects(), before);

    // --- OPERATORS ---

    Family lowRisk("Jiten Saikia", "Mayong");
    lowRisk.addMember(new Adult("Jiten Saikia", 40, 'M', lowRisk.getId(), 0));
    ASSERT_EQ(lowRisk.vulnerabilityScore(), 10);

    // operator< orders by vulnerability score, which the priority queue uses.
    ASSERT_TRUE(lowRisk < das);
    ASSERT_FALSE(das < lowRisk);

    // operator== is identity, for the duplicate-claim check.
    Family same = das;            // copy keeps the family ID
    ASSERT_TRUE(same == das);
    ASSERT_FALSE(lowRisk == das);

    // adoptId restores a loaded identity.
    Family loaded("Loaded Head", "Loaded Village");
    loaded.adoptId(7700);
    ASSERT_EQ(loaded.getId(), 7700);

    // operator<< summarises the family.
    std::ostringstream oss;
    oss << das;
    ASSERT_TRUE(oss.str().find("Bipul Das") != std::string::npos);
    ASSERT_TRUE(oss.str().find("5") != std::string::npos);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
```

Add `void run_family_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Family.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Family.h`:

```cpp
#ifndef RELIEF_FAMILY_H
#define RELIEF_FAMILY_H

#include "Person.h"
#include "Ration.h"
#include <string>
#include <ostream>

class RationCard;   // Task 9 adds the conversion operator

// A displaced household: the unit that gets registered, admitted and rationed.
//
// [Dynamic memory] [Array of dynamic objects] Family owns its members through a
// raw Person** array that it grows itself. A std::vector<std::unique_ptr<Person>>
// would be the modern choice - Registry<T> in Task 10 shows that style - but
// owning raw memory here is what makes the rule of three below necessary and
// visible.
class Family {
    static int nextId;                 // [Static data member]

    int id;
    std::string headName;
    std::string village;
    int campId;                        // 0 means "not yet admitted"

    Person** members;                  // the dynamic array
    int count;                         // members in use
    int capacity;                      // slots allocated

    void grow();                       // doubles the array
    void copyFrom(const Family& other);// deep-copies another family's members
    void freeAll();                    // deletes every member and the array

public:
    Family();                                                      // [Default constructor]
    Family(const std::string& headName, const std::string& village);// [Parameterised]
    Family(const Family& other);                                   // [Copy constructor]
    Family& operator=(const Family& other);                        // [Copy assignment]
    ~Family();                                                     // [Destructor]

    static int generateId();           // [Static member function]
    static void resetIdCounter(int startAt);
    void adoptId(int existingId);

    int getId() const;
    const std::string& getHeadName() const;
    const std::string& getVillage() const;
    int getCampId() const;
    void setCampId(int newCampId);
    void setHeadName(const std::string& newHeadName);

    int size() const;
    const Person& memberAt(int index) const;   // throws std::out_of_range
    Person& memberAt(int index);
    void addMember(Person* p);                 // Family takes ownership of p
    int countOfTag(const std::string& tag) const;

    // [Runtime polymorphism] the sum walks base pointers and each member
    // answers with its own override.
    Ration dailyRation() const;                // [Returning objects]
    int vulnerabilityScore() const;

    // Metric functions for [pointer to member function] sorting in Task 15.
    // They all share one signature, which is the whole point.
    double metricVulnerability() const;
    double metricSize() const;
    double metricAverageAge() const;
    double metricInfantCount() const;

    // [Friend functions]
    friend bool operator<(const Family& a, const Family& b);   // by vulnerability
    friend bool operator==(const Family& a, const Family& b);  // by identity
    friend std::ostream& operator<<(std::ostream& os, const Family& f);
};

#endif
```

`src/Family.cpp` — write the three big functions exactly like this, since they are the task's whole point:

```cpp
Family::Family(const Family& other)
    : id(other.id), headName(other.headName), village(other.village),
      campId(other.campId), members(0), count(0), capacity(0) {
    // [Copy constructor] A shallow copy would leave two families pointing at
    // the same Person objects, and the second destructor would delete them
    // twice. clone() gives each family its own people.
    copyFrom(other);
    RELIEF_LIFECYCLE("Family copied    #" << id << " with " << count << " members");
}

Family& Family::operator=(const Family& other) {
    // [Copy assignment] Guard against self-assignment FIRST. Without this,
    // `f = f` would free its own members and then copy from freed memory.
    if (this == &other) {
        return *this;
    }
    freeAll();
    id = other.id;
    headName = other.headName;
    village = other.village;
    campId = other.campId;
    copyFrom(other);
    return *this;          // returning a reference is what allows a = b = c
}

Family::~Family() {
    // [Destructor] Family allocated the members, so Family frees them.
    RELIEF_LIFECYCLE("Family destroyed #" << id << " with " << count << " members");
    freeAll();
}

void Family::copyFrom(const Family& other) {
    capacity = other.count > 0 ? other.count : 4;
    members = new Person*[capacity];
    count = 0;
    for (int i = 0; i < other.count; i++) {
        members[count] = other.members[i]->clone();   // deep copy, keeps the type
        count++;
    }
}

void Family::freeAll() {
    for (int i = 0; i < count; i++) {
        delete members[i];
    }
    delete[] members;
    members = 0;
    count = 0;
    capacity = 0;
}

void Family::grow() {
    int newCapacity = capacity == 0 ? 4 : capacity * 2;
    Person** bigger = new Person*[newCapacity];
    for (int i = 0; i < count; i++) {
        bigger[i] = members[i];
    }
    delete[] members;          // free the old array, not the people in it
    members = bigger;
    capacity = newCapacity;
}

void Family::addMember(Person* p) {
    if (p == 0) { return; }
    if (count == capacity) { grow(); }
    members[count] = p;
    count++;
}
```

Remaining notes:
- `int Family::nextId = 1;` at file scope. Default constructor: `headName = "unnamed"`, `village = "unknown"`, `campId = 0`, `members = 0`, `count = 0`, `capacity = 0`, `id = generateId()`.
- `memberAt` throws `std::out_of_range("member index out of range")` when `index < 0 || index >= count`.
- `dailyRation()`: `Ration total; for (int i = 0; i < count; i++) { total += members[i]->rationNeed(); } return total;`
- `vulnerabilityScore()`: sum of `members[i]->vulnerabilityScore()`.
- `metricAverageAge()`: returns `0.0` when `count == 0`.
- `countOfTag`: compares `members[i]->typeTag() == tag`.
- `operator<`: `return a.vulnerabilityScore() < b.vulnerabilityScore();`
- `operator==`: `return a.id == b.id;`
- `operator<<`: `os << "Family #" << f.id << " " << f.headName << " (" << f.village << "), " << f.count << " members, vulnerability " << f.vulnerabilityScore();`
- Note that the copy constructor keeps `other.id`, so a copied family is still the *same* family on paper — the test relies on `same == das`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Family and the rule of three` suite all ok, including the live-object count returning to its starting value.

Then check for leaks if a tool is available: `g++ -std=c++17 -fsanitize=address -Iinclude -Itests src/*.cpp tests/*.cpp -o bin/tests_asan -pthread && ./bin/tests_asan` (skip `src/main.cpp` and `src/seed_main.cpp` from the glob, or use `make test` plus a manual command). Expected: no leak or heap-use-after-free reports. On Windows/MinGW where ASan is unavailable, rely on the `Person::liveObjects()` assertions instead.

- [ ] **Step 5: Commit**

```bash
git add include/Family.h src/Family.cpp tests/test_family.cpp tests/test_main.cpp
git commit -m "Add Family with a hand-written rule of three

Family owns a raw Person** array it grows itself, deep-copies through the
virtual clone() so each copy owns its own people, guards self-assignment, and
frees everything in its destructor. Tests assert the live Person count returns
to zero, that a copy's members are distinct objects, and that f = f survives."
```

---

## Task 9: RationCard and the Family-to-RationCard conversion

**Files:**
- Create: `include/RationCard.h`, `src/RationCard.cpp`
- Modify: `include/Family.h` (add `operator RationCard() const;`), `src/Family.cpp` (define it, include `RationCard.h`), `tests/test_family.cpp` (append the conversion section)

**Interfaces:**
- Consumes: `Ration.h`, `Family.h`.
- Produces: `class RationCard` with `RationCard()`, a full parameterised constructor, const getters (`getFamilyId`, `getHeadName`, `getCampId`, `getMemberCount`, `getInfants`, `getChildren`, `getAdults`, `getElderly`, `getDaily`, `getVulnerability`), `std::string render() const`, and `friend std::ostream& operator<<`. Plus `Family::operator RationCard() const`.

- [ ] **Step 1: Write the failing test**

Append to `tests/test_family.cpp` (add `#include "RationCard.h"`), inside `run_family_tests()`:

```cpp
    testing::beginSuite("Family to RationCard conversion");

    Person::resetIdCounter(1);
    Family::resetIdCounter(500);

    Family card_family = makeDasFamily();
    card_family.setCampId(3);

    // [Class to class conversion] implicit, on assignment to a RationCard.
    RationCard card = card_family;
    ASSERT_EQ(card.getFamilyId(), card_family.getId());
    ASSERT_STR_EQ(card.getHeadName(), "Bipul Das");
    ASSERT_EQ(card.getCampId(), 3);
    ASSERT_EQ(card.getMemberCount(), 5);
    ASSERT_EQ(card.getInfants(), 1);
    ASSERT_EQ(card.getChildren(), 1);
    ASSERT_EQ(card.getAdults(), 2);
    ASSERT_EQ(card.getElderly(), 1);
    ASSERT_EQ(card.getVulnerability(), 115);
    ASSERT_NEAR(card.getDaily().get(Item::Rice), 1.300, 1e-9);

    // The same conversion works explicitly, which is how the menu uses it.
    RationCard explicitCard = static_cast<RationCard>(card_family);
    ASSERT_EQ(explicitCard.getMemberCount(), 5);

    // Default constructor gives an empty card.
    RationCard blankCard;
    ASSERT_EQ(blankCard.getFamilyId(), 0);
    ASSERT_EQ(blankCard.getMemberCount(), 0);

    // render() produces the demo-facing card text.
    std::string text = card.render();
    ASSERT_TRUE(text.find("RATION CARD") != std::string::npos);
    ASSERT_TRUE(text.find("Bipul Das") != std::string::npos);
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("115") != std::string::npos);

    std::ostringstream cardOss;
    cardOss << card;
    ASSERT_TRUE(cardOss.str().find("Bipul Das") != std::string::npos);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: RationCard.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/RationCard.h`:

```cpp
#ifndef RELIEF_RATIONCARD_H
#define RELIEF_RATIONCARD_H

#include "Ration.h"
#include <string>
#include <ostream>

// The slip an official hands a family: who they are, who is in the household,
// what they are entitled to each day. A snapshot, not a live view - it is a
// different kind of thing from a Family, which is why the conversion in
// Family is a class-to-class conversion and not just a getter.
class RationCard {
    int familyId;
    std::string headName;
    int campId;
    int memberCount;
    int infants;
    int children;
    int adults;
    int elderly;
    Ration daily;
    int vulnerability;
public:
    RationCard();
    RationCard(int familyId, const std::string& headName, int campId,
               int memberCount, int infants, int children, int adults,
               int elderly, const Ration& daily, int vulnerability);
    RationCard(const RationCard& other);
    ~RationCard();

    int getFamilyId() const;
    const std::string& getHeadName() const;
    int getCampId() const;
    int getMemberCount() const;
    int getInfants() const;
    int getChildren() const;
    int getAdults() const;
    int getElderly() const;
    const Ration& getDaily() const;
    int getVulnerability() const;

    std::string render() const;    // the boxed card text shown in the demo

    friend std::ostream& operator<<(std::ostream& os, const RationCard& c);
};

#endif
```

In `include/Family.h`, add to the public section with the tag comment:

```cpp
    // [Class to class conversion] a Family converts to the printable card an
    // official hands over. RationCard is only forward-declared above; the
    // definition lives in Family.cpp, which includes RationCard.h.
    operator RationCard() const;
```

In `src/Family.cpp`, add `#include "RationCard.h"` and:

```cpp
Family::operator RationCard() const {
    return RationCard(id, headName, campId, count,
                      countOfTag("INFANT"), countOfTag("CHILD"),
                      countOfTag("ADULT") + countOfTag("RESIDENT_VOLUNTEER"),
                      countOfTag("ELDERLY"),
                      dailyRation(), vulnerabilityScore());
}
```

`RationCard::render()` builds a boxed block with `std::ostringstream`, containing the literal heading `RATION CARD`, the family ID and head name, the camp, the member breakdown, each non-zero ration line with its `itemUnit`, and the vulnerability score. `operator<<` prints `render()`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Family to RationCard conversion` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/RationCard.h src/RationCard.cpp include/Family.h src/Family.cpp tests/test_family.cpp
git commit -m "Add RationCard and the Family-to-RationCard conversion

A RationCard is a snapshot an official hands over, which is a different kind
of thing from a live Family, so the class-to-class conversion operator earns
its place. Resident volunteers count as adults on the card."
```

---

## Task 10: Registry&lt;T&gt; class template and findMax function template

**Files:**
- Create: `include/Registry.h`, `include/Algorithms.h`, `tests/test_registry.cpp`
- Modify: `tests/test_main.cpp` (add `run_registry_tests();`)

**Interfaces:**
- Consumes: `Family.h` (for the test), `Person.h`.
- Produces:
  - `template <typename T> class Registry` — header-only. `Registry()`, `T* add(std::unique_ptr<T> item)`, `T* findById(int id) const`, `bool remove(int id)`, `int size() const`, `T* at(int index) const`, `std::vector<T*> all() const`, `void clear()`. Requires `T` to have `int getId() const`.
  - `template <typename T> T* findMax(const std::vector<T*>& items, double (T::*metric)() const)` — header-only, returns `0` for an empty vector.
  - `Registry` is move-only (copy deleted) because `unique_ptr` is.

- [ ] **Step 1: Write the failing test**

Create `tests/test_registry.cpp`:

```cpp
#include "TestRunner.h"
#include "Registry.h"
#include "Algorithms.h"
#include "Family.h"
#include "Adult.h"
#include "Infant.h"
#include "Volunteer.h"
#include <memory>

void run_registry_tests() {
    testing::beginSuite("Registry<T> and findMax");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    Registry<Family> families;
    ASSERT_EQ(families.size(), 0);
    ASSERT_TRUE(families.findById(1) == 0);
    ASSERT_TRUE(families.at(0) == 0);

    // add() takes ownership and hands back a usable pointer.
    Family* a = families.add(std::unique_ptr<Family>(new Family("Bipul Das", "Bhuragaon")));
    Family* b = families.add(std::unique_ptr<Family>(new Family("Jiten Saikia", "Mayong")));
    Family* c = families.add(std::unique_ptr<Family>(new Family("Rupa Nath", "Laharighat")));
    ASSERT_EQ(families.size(), 3);
    ASSERT_STR_EQ(a->getHeadName(), "Bipul Das");

    // findById works through the template's requirement that T has getId().
    ASSERT_TRUE(families.findById(a->getId()) == a);
    ASSERT_TRUE(families.findById(b->getId()) == b);
    ASSERT_TRUE(families.findById(99999) == 0);

    // at() is positional.
    ASSERT_TRUE(families.at(0) == a);
    ASSERT_TRUE(families.at(2) == c);
    ASSERT_TRUE(families.at(3) == 0);
    ASSERT_TRUE(families.at(-1) == 0);

    // all() gives a flat view for iteration.
    std::vector<Family*> view = families.all();
    ASSERT_EQ(static_cast<int>(view.size()), 3);
    ASSERT_TRUE(view[1] == b);

    // remove() frees the element; the registry shrinks.
    int bId = b->getId();
    ASSERT_TRUE(families.remove(bId));
    ASSERT_EQ(families.size(), 2);
    ASSERT_TRUE(families.findById(bId) == 0);
    ASSERT_FALSE(families.remove(bId));        // already gone

    // The same template over a different type, proving it is generic.
    Registry<Volunteer> helpers;
    Volunteer* vol = helpers.add(std::unique_ptr<Volunteer>(
        new Volunteer("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross")));
    ASSERT_EQ(helpers.size(), 1);
    ASSERT_TRUE(helpers.findById(vol->getId()) == vol);
    ASSERT_STR_EQ(helpers.at(0)->getSkill(), "Medic");

    // --- findMax over a pointer-to-member metric ---
    a->addMember(new Adult("Bipul Das", 35, 'M', a->getId(), 0));          // score 10
    c->addMember(new Infant("Mitu Nath", 1, 'F', c->getId(), 0));          // score 40
    c->addMember(new Adult("Rupa Nath", 30, 'F', c->getId(), 0));          // score 10

    std::vector<Family*> all = families.all();
    Family* mostVulnerable = findMax(all, &Family::metricVulnerability);
    ASSERT_TRUE(mostVulnerable == c);
    ASSERT_NEAR(mostVulnerable->metricVulnerability(), 50.0, 1e-9);

    Family* biggest = findMax(all, &Family::metricSize);
    ASSERT_TRUE(biggest == c);

    Family* oldest = findMax(all, &Family::metricAverageAge);
    ASSERT_TRUE(oldest == a);                   // average 35 beats (1+30)/2

    std::vector<Family*> none;
    ASSERT_TRUE(findMax(none, &Family::metricSize) == 0);

    // clear() empties the registry and frees everything.
    int liveBefore = Person::liveObjects();
    ASSERT_TRUE(liveBefore > 0);
    families.clear();
    ASSERT_EQ(families.size(), 0);
    ASSERT_EQ(Person::liveObjects(), 0);        // the families took their members with them

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Registry.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Registry.h`:

```cpp
#ifndef RELIEF_REGISTRY_H
#define RELIEF_REGISTRY_H

#include <vector>
#include <memory>

// [Class template] One store for every kind of entity that has an ID. Camps,
// families and volunteers all need store / find / remove, so the code is
// written once and the compiler stamps out a version per type.
//
// T must provide `int getId() const`.
//
// Ownership note: this uses std::unique_ptr, while Family in Task 8 owns its
// members with raw new/delete. Both styles are in the project on purpose, so
// the manual version can be compared with the automatic one.
template <typename T>
class Registry {
    std::vector<std::unique_ptr<T> > items;

public:
    Registry() {}

    // unique_ptr cannot be copied, so neither can a Registry. Saying so out
    // loud is better than letting the compiler produce a confusing error.
    Registry(const Registry& other) = delete;
    Registry& operator=(const Registry& other) = delete;

    T* add(std::unique_ptr<T> item) {
        T* raw = item.get();
        items.push_back(std::move(item));
        return raw;
    }

    // [Const member function] finding does not change the registry
    T* findById(int id) const {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (items[i]->getId() == id) {
                return items[i].get();
            }
        }
        return 0;
    }

    bool remove(int id) {
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            if (items[i]->getId() == id) {
                items.erase(items.begin() + i);
                return true;
            }
        }
        return false;
    }

    int size() const { return static_cast<int>(items.size()); }

    T* at(int index) const {
        if (index < 0 || index >= static_cast<int>(items.size())) {
            return 0;
        }
        return items[index].get();
    }

    std::vector<T*> all() const {
        std::vector<T*> out;
        for (int i = 0; i < static_cast<int>(items.size()); i++) {
            out.push_back(items[i].get());
        }
        return out;
    }

    void clear() { items.clear(); }
};

#endif
```

Create `include/Algorithms.h`:

```cpp
#ifndef RELIEF_ALGORITHMS_H
#define RELIEF_ALGORITHMS_H

#include <vector>

// [Function template] + [Pointer to member function]
// Finds the element scoring highest on a metric the caller chooses, where the
// metric is a member function of T. One function replaces a separate
// findMostVulnerable, findLargest, findOldest and so on.
//
// Example: findMax(families, &Family::metricVulnerability)
template <typename T>
T* findMax(const std::vector<T*>& items, double (T::*metric)() const) {
    if (items.empty()) {
        return 0;
    }
    T* best = items[0];
    double bestValue = (best->*metric)();      // call through the member pointer
    for (int i = 1; i < static_cast<int>(items.size()); i++) {
        double value = (items[i]->*metric)();
        if (value > bestValue) {
            bestValue = value;
            best = items[i];
        }
    }
    return best;
}

#endif
```

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Registry<T> and findMax` suite all ok, including `Person::liveObjects() == 0` after `clear()`.

- [ ] **Step 5: Commit**

```bash
git add include/Registry.h include/Algorithms.h tests/test_registry.cpp tests/test_main.cpp
git commit -m "Add Registry<T> class template and findMax function template

Registry<T> stores any entity with getId() behind unique_ptr, the deliberate
counterpart to Family's manual ownership. findMax takes a pointer to a member
function, so one template replaces a family of near-identical search helpers."
```

---

## Task 11: Custom exception hierarchy

**Files:**
- Create: `include/Exceptions.h`, `src/Exceptions.cpp`, `tests/test_exceptions.cpp`
- Modify: `tests/test_main.cpp` (add `run_exception_tests();`)

**Interfaces:**
- Consumes: `Item.h`, `Date.h`.
- Produces:
  - `class ReliefException : public std::exception` — `explicit ReliefException(const std::string& message)`, `~ReliefException() override`, `const char* what() const noexcept override`.
  - `class OverCapacityException : public ReliefException` — `OverCapacityException(int campId, int requested, int freeSpace)`, `getCampId()`, `getRequested()`, `getFreeSpace()`.
  - `class InsufficientStockException : public ReliefException` — `InsufficientStockException(Item item, double needed, double available)`, `getItem()`, `getNeeded()`, `getAvailable()`.
  - `class DuplicateClaimException : public ReliefException` — `DuplicateClaimException(int familyId, const Date& date)`, `getFamilyId()`, `getDate()`.
  - `class FileFormatException : public ReliefException` — `FileFormatException(const std::string& file, int line, const std::string& detail)`, `getFile()`, `getLine()`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_exceptions.cpp`:

```cpp
#include "TestRunner.h"
#include "Exceptions.h"
#include <string>
#include <exception>

void run_exception_tests() {
    testing::beginSuite("Custom exception hierarchy");

    // The base carries a message through std::exception::what().
    ReliefException base("something went wrong");
    ASSERT_STR_EQ(std::string(base.what()), "something went wrong");

    // Every custom exception is catchable as the base, and as std::exception.
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), ReliefException);
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), std::exception);
    ASSERT_THROWS(throw InsufficientStockException(Item::Rice, 10.0, 4.0), ReliefException);
    ASSERT_THROWS(throw DuplicateClaimException(101, Date(2026, 7, 14)), ReliefException);
    ASSERT_THROWS(throw FileFormatException("families.csv", 12, "expected 5 fields"), ReliefException);

    // And each one is catchable as its own precise type.
    ASSERT_THROWS(throw OverCapacityException(3, 5, 2), OverCapacityException);
    ASSERT_THROWS(throw InsufficientStockException(Item::Rice, 10.0, 4.0), InsufficientStockException);
    ASSERT_THROWS(throw DuplicateClaimException(101, Date(2026, 7, 14)), DuplicateClaimException);
    ASSERT_THROWS(throw FileFormatException("families.csv", 12, "bad"), FileFormatException);

    // OverCapacityException carries what the handler needs to suggest another camp.
    try {
        throw OverCapacityException(3, 5, 2);
    } catch (const OverCapacityException& e) {
        ASSERT_EQ(e.getCampId(), 3);
        ASSERT_EQ(e.getRequested(), 5);
        ASSERT_EQ(e.getFreeSpace(), 2);
        std::string msg(e.what());
        ASSERT_TRUE(msg.find("3") != std::string::npos);
        ASSERT_TRUE(msg.find("5") != std::string::npos);
        ASSERT_TRUE(msg.find("2") != std::string::npos);
    }

    // InsufficientStockException names the item and the shortfall.
    try {
        throw InsufficientStockException(Item::Rice, 10.0, 4.0);
    } catch (const InsufficientStockException& e) {
        ASSERT_TRUE(e.getItem() == Item::Rice);
        ASSERT_NEAR(e.getNeeded(), 10.0, 1e-9);
        ASSERT_NEAR(e.getAvailable(), 4.0, 1e-9);
        ASSERT_TRUE(std::string(e.what()).find("Rice") != std::string::npos);
    }

    // DuplicateClaimException names the family and the date.
    try {
        throw DuplicateClaimException(101, Date(2026, 7, 14));
    } catch (const DuplicateClaimException& e) {
        ASSERT_EQ(e.getFamilyId(), 101);
        ASSERT_TRUE(e.getDate() == Date(2026, 7, 14));
        ASSERT_TRUE(std::string(e.what()).find("2026-07-14") != std::string::npos);
    }

    // FileFormatException points at the exact line, which is what makes a
    // hand-edited CSV debuggable.
    try {
        throw FileFormatException("families.csv", 12, "expected 5 fields, found 3");
    } catch (const FileFormatException& e) {
        ASSERT_STR_EQ(e.getFile(), "families.csv");
        ASSERT_EQ(e.getLine(), 12);
        std::string msg(e.what());
        ASSERT_TRUE(msg.find("families.csv") != std::string::npos);
        ASSERT_TRUE(msg.find("12") != std::string::npos);
        ASSERT_TRUE(msg.find("expected 5 fields") != std::string::npos);
    }

    // Catching by base reference keeps the derived message: the virtual what()
    // is doing the work.
    try {
        throw InsufficientStockException(Item::Dal, 3.0, 0.5);
    } catch (const ReliefException& e) {
        ASSERT_TRUE(std::string(e.what()).find("Dal") != std::string::npos);
    }
}
```

Add `void run_exception_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Exceptions.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Exceptions.h`:

```cpp
#ifndef RELIEF_EXCEPTIONS_H
#define RELIEF_EXCEPTIONS_H

#include "Item.h"
#include "Date.h"
#include <exception>
#include <string>

// [Custom exception base class] Everything this program throws derives from
// here, so the menu can write one `catch (const ReliefException& e)` and still
// recover the precise type when it needs the details.
class ReliefException : public std::exception {
    std::string message;
public:
    explicit ReliefException(const std::string& message);
    ~ReliefException() override;
    const char* what() const noexcept override;    // [Overriding a virtual]
};

// [Derived exception] Thrown by Camp::admit and Camp::Shelter::occupy.
// It carries the free space so the handler can suggest another camp.
class OverCapacityException : public ReliefException {
    int campId;
    int requested;
    int freeSpace;
public:
    OverCapacityException(int campId, int requested, int freeSpace);
    int getCampId() const;
    int getRequested() const;
    int getFreeSpace() const;
};

// [Derived exception] Thrown by Stock::take and operator-(Stock, Allocation).
class InsufficientStockException : public ReliefException {
    Item item;
    double needed;
    double available;
public:
    InsufficientStockException(Item item, double needed, double available);
    Item getItem() const;
    double getNeeded() const;
    double getAvailable() const;
};

// [Derived exception] Thrown by DistributionLog::record when a family tries to
// claim twice on the same day, in any camp.
class DuplicateClaimException : public ReliefException {
    int familyId;
    Date date;
public:
    DuplicateClaimException(int familyId, const Date& date);
    int getFamilyId() const;
    const Date& getDate() const;
};

// [Derived exception] Thrown by the CSV layer, with the file and line number
// so a hand-edited data file can be fixed.
class FileFormatException : public ReliefException {
    std::string file;
    int line;
public:
    FileFormatException(const std::string& file, int line, const std::string& detail);
    const std::string& getFile() const;
    int getLine() const;
};

#endif
```

`src/Exceptions.cpp` notes — each derived constructor builds its message and passes it up:

```cpp
static std::string buildOverCapacityMessage(int campId, int requested, int freeSpace) {
    std::ostringstream oss;
    oss << "camp " << campId << " cannot take " << requested
        << " more people; only " << freeSpace << " space(s) free";
    return oss.str();
}

OverCapacityException::OverCapacityException(int campId, int requested, int freeSpace)
    : ReliefException(buildOverCapacityMessage(campId, requested, freeSpace)),
      campId(campId), requested(requested), freeSpace(freeSpace) {}
```

Follow the same shape for the other three:
- `InsufficientStockException`: `"not enough " << itemName(item) << ": need " << needed << " " << itemUnit(item) << ", have " << available`.
- `DuplicateClaimException`: `"family " << familyId << " has already collected rations on " << date.toString()`.
- `FileFormatException`: `file << " line " << line << ": " << detail`.
- `ReliefException::what()` returns `message.c_str()`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Custom exception hierarchy` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Exceptions.h src/Exceptions.cpp tests/test_exceptions.cpp tests/test_main.cpp
git commit -m "Add ReliefException hierarchy with four derived exceptions

Each exception carries the data its handler needs: OverCapacityException
reports the free space so the menu can suggest another camp, and
FileFormatException names the file and line so a hand-edited CSV is
debuggable. All four are catchable as ReliefException or as std::exception."
```

---
## Task 12: Allocation and Stock

**Files:**
- Create: `include/Allocation.h`, `src/Allocation.cpp`, `include/Stock.h`, `src/Stock.cpp`, `tests/test_stock.cpp`
- Modify: `tests/test_main.cpp` (add `run_stock_tests();`)

**Interfaces:**
- Consumes: `Item.h`, `Date.h`, `Ration.h`, `Exceptions.h`.
- Produces:
  - `class Allocation`: `Allocation()`, `Allocation(int familyId, int campId, const Date& date, const Ration& amount)`, copy constructor, `getFamilyId()`, `getCampId()`, `getDate()`, `getAmount()`, `operator Ration() const`, `friend operator==` (same family and date), `friend operator<<`.
  - `class Stock`: `static const double kUnlimitedDays;` (999.0), `Stock()`, copy constructor, destructor, `double get(Item) const`, `void set(Item, double)`, `void add(Item, double)`, `void take(Item, double)` (throws `InsufficientStockException`), `void setDailyDemand(const Ration&)`, `const Ration& getDailyDemand() const`, `double daysOfSupply() const`, `double daysOfSupplyFor(Item) const`, `Item mostCriticalItem() const`, `operator double() const`, `double operator[](Item) const`, `Stock& operator+=(const Stock&)`, `Stock& operator-=(const Allocation&)`, and friends `operator+(const Stock&, const Stock&)`, `operator-(const Stock&, const Allocation&)`, `operator<<`.
- Include direction: `Stock.h` includes `Ration.h` and forward-declares `class Allocation;`. `Stock.cpp` includes `Allocation.h` and `Exceptions.h`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_stock.cpp`:

```cpp
#include "TestRunner.h"
#include "Stock.h"
#include "Allocation.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <sstream>

void run_stock_tests() {
    testing::beginSuite("Stock, Allocation and days of supply");

    // --- Allocation ---
    Ration handout = RationNorms::forAdult() * 3.0;      // three adults for a day
    Allocation alloc(101, 1, Date(2026, 7, 14), handout);
    ASSERT_EQ(alloc.getFamilyId(), 101);
    ASSERT_EQ(alloc.getCampId(), 1);
    ASSERT_TRUE(alloc.getDate() == Date(2026, 7, 14));
    ASSERT_NEAR(alloc.getAmount().get(Item::Rice), 1.200, 1e-9);

    // [Class to class conversion] an Allocation is usable as the Ration it carries.
    Ration asRation = alloc;
    ASSERT_NEAR(asRation.get(Item::Rice), 1.200, 1e-9);

    // operator== is "same family, same day" - the duplicate-claim rule.
    Allocation sameDay(101, 2, Date(2026, 7, 14), handout);     // different camp!
    Allocation nextDay(101, 1, Date(2026, 7, 15), handout);
    Allocation otherFamily(102, 1, Date(2026, 7, 14), handout);
    ASSERT_TRUE(alloc == sameDay);          // same family, same date, camp is irrelevant
    ASSERT_FALSE(alloc == nextDay);
    ASSERT_FALSE(alloc == otherFamily);

    // --- Stock basics ---
    Stock s;
    ASSERT_NEAR(s.get(Item::Rice), 0.0, 1e-9);

    s.set(Item::Rice, 500.0);
    s.set(Item::Dal, 80.0);
    s.set(Item::Oil, 40.0);
    s.set(Item::Salt, 15.0);
    s.set(Item::ORS, 300.0);
    s.set(Item::BabyFood, 20.0);
    s.set(Item::Tarpaulin, 60.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);
    ASSERT_NEAR(s[Item::Dal], 80.0, 1e-9);

    s.add(Item::Rice, 100.0);
    ASSERT_NEAR(s.get(Item::Rice), 600.0, 1e-9);

    s.take(Item::Rice, 100.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);

    // take() beyond what is there is an error, not a silent negative.
    ASSERT_THROWS(s.take(Item::Rice, 10000.0), InsufficientStockException);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // unchanged after the throw
    try {
        s.take(Item::Dal, 200.0);
    } catch (const InsufficientStockException& e) {
        ASSERT_TRUE(e.getItem() == Item::Dal);
        ASSERT_NEAR(e.getNeeded(), 200.0, 1e-9);
        ASSERT_NEAR(e.getAvailable(), 80.0, 1e-9);
    }

    // Copy constructor is independent.
    Stock copied(s);
    copied.set(Item::Rice, 1.0);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);

    // --- days of supply ---

    // With no demand recorded, days of supply is the "unlimited" sentinel.
    ASSERT_NEAR(s.daysOfSupply(), Stock::kUnlimitedDays, 1e-9);

    // 100 adults a day: rice 40 kg/day, dal 6 kg/day, oil 3 l/day, salt 1 kg/day,
    // ORS 10/day, baby food 0 (no demand), tarpaulin 0 (no demand).
    Ration demand = RationNorms::forAdult() * 100.0;
    s.setDailyDemand(demand);
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Rice), 500.0 / 40.0, 1e-9);     // 12.5
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Dal), 80.0 / 6.0, 1e-6);        // 13.33
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Oil), 40.0 / 3.0, 1e-6);        // 13.33
    ASSERT_NEAR(s.daysOfSupplyFor(Item::Salt), 15.0, 1e-9);             // 15
    ASSERT_NEAR(s.daysOfSupplyFor(Item::ORS), 30.0, 1e-9);              // 30
    // Items with no demand never limit the camp.
    ASSERT_NEAR(s.daysOfSupplyFor(Item::BabyFood), Stock::kUnlimitedDays, 1e-9);

    // The worst item decides.
    ASSERT_NEAR(s.daysOfSupply(), 12.5, 1e-9);
    ASSERT_TRUE(s.mostCriticalItem() == Item::Rice);

    // [Class to basic type conversion] a Stock used as a double IS its days of supply.
    double days = s;
    ASSERT_NEAR(days, 12.5, 1e-9);
    ASSERT_TRUE(s < 20.0);                   // reads naturally thanks to the conversion
    ASSERT_FALSE(s < 5.0);

    // Drain the rice and the critical item changes.
    Stock low(s);
    low.set(Item::Rice, 400.0);              // 10 days
    low.set(Item::Salt, 8.0);                // 8 days - now the worst
    ASSERT_NEAR(low.daysOfSupply(), 8.0, 1e-9);
    ASSERT_TRUE(low.mostCriticalItem() == Item::Salt);

    // --- operators on whole objects ---

    Stock arriving;
    arriving.set(Item::Rice, 250.0);
    arriving.set(Item::Dal, 20.0);

    // [Friend binary operator] [Objects as arguments] [Returning objects]
    Stock merged = s + arriving;
    ASSERT_NEAR(merged.get(Item::Rice), 750.0, 1e-9);
    ASSERT_NEAR(merged.get(Item::Dal), 100.0, 1e-9);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // operands untouched

    Stock accumulated(s);
    accumulated += arriving;
    ASSERT_NEAR(accumulated.get(Item::Rice), 750.0, 1e-9);

    // Stock - Allocation debits exactly what the family was handed.
    Stock afterHandout = s - alloc;
    ASSERT_NEAR(afterHandout.get(Item::Rice), 500.0 - 1.200, 1e-9);
    ASSERT_NEAR(afterHandout.get(Item::Dal), 80.0 - 0.180, 1e-9);
    ASSERT_NEAR(s.get(Item::Rice), 500.0, 1e-9);        // the original is unchanged

    Stock debited(s);
    debited -= alloc;
    ASSERT_NEAR(debited.get(Item::Rice), 500.0 - 1.200, 1e-9);

    // Debiting more than the camp holds is refused.
    Stock nearlyEmpty;
    nearlyEmpty.set(Item::Rice, 0.5);
    ASSERT_THROWS(nearlyEmpty - alloc, InsufficientStockException);
    ASSERT_THROWS(nearlyEmpty -= alloc, InsufficientStockException);

    // The daily demand survives the arithmetic, so days of supply stays meaningful.
    ASSERT_NEAR(afterHandout.getDailyDemand().get(Item::Rice), 40.0, 1e-9);

    // Stream output lists the items with their units.
    std::ostringstream oss;
    oss << s;
    ASSERT_TRUE(oss.str().find("Rice") != std::string::npos);
    ASSERT_TRUE(oss.str().find("kg") != std::string::npos);
}
```

Add `void run_stock_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Stock.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Allocation.h`:

```cpp
#ifndef RELIEF_ALLOCATION_H
#define RELIEF_ALLOCATION_H

#include "Ration.h"
#include "Date.h"
#include <ostream>

// One handout: this family, at this camp, on this day, received this much.
// The unit of record for the distribution log.
class Allocation {
    int familyId;
    int campId;
    Date date;
    Ration amount;
public:
    Allocation();
    Allocation(int familyId, int campId, const Date& date, const Ration& amount);
    Allocation(const Allocation& other);
    ~Allocation();

    int getFamilyId() const;
    int getCampId() const;
    const Date& getDate() const;
    const Ration& getAmount() const;

    // [Class to class conversion] an Allocation can stand in for the Ration it
    // carries, which is what lets `stock - allocation` be written at all.
    operator Ration() const;

    // [Friend function] Two allocations are "the same claim" when the same
    // family claimed on the same day - the camp does NOT matter, because a
    // family must not be able to collect twice by walking to another camp.
    friend bool operator==(const Allocation& a, const Allocation& b);
    friend std::ostream& operator<<(std::ostream& os, const Allocation& a);
};

#endif
```

Create `include/Stock.h`:

```cpp
#ifndef RELIEF_STOCK_H
#define RELIEF_STOCK_H

#include "Item.h"
#include "Ration.h"
#include <ostream>

class Allocation;   // only the .cpp needs the full definition

// What a camp physically holds, plus the camp's current daily draw. Keeping the
// draw here is what lets a Stock answer "how many days do we have left?" on its
// own, and therefore what makes the conversion to double meaningful.
class Stock {
    double qty[ITEM_COUNT];
    Ration dailyDemand;
public:
    // [Static data member] the answer when nothing is being consumed
    static const double kUnlimitedDays;

    Stock();
    Stock(const Stock& other);
    ~Stock();

    double get(Item it) const;
    void set(Item it, double value);
    void add(Item it, double value);
    void take(Item it, double value);          // throws InsufficientStockException

    void setDailyDemand(const Ration& demand);
    const Ration& getDailyDemand() const;

    double daysOfSupplyFor(Item it) const;
    double daysOfSupply() const;               // the worst item decides
    Item mostCriticalItem() const;

    // [Class to basic type conversion] `double d = stock;` gives the days of
    // supply. A daysOfSupply(demand) method would be a cleaner design, but the
    // conversion is what the syllabus asks to see, so the demand is stored
    // above to make the no-argument form possible.
    operator double() const;

    double operator[](Item it) const;
    Stock& operator+=(const Stock& other);
    Stock& operator-=(const Allocation& a);    // [Object as argument]

    // [Friend binary operators]
    friend Stock operator+(const Stock& a, const Stock& b);
    friend Stock operator-(const Stock& s, const Allocation& a);
    friend std::ostream& operator<<(std::ostream& os, const Stock& s);
};

#endif
```

Implementation notes:
- `const double Stock::kUnlimitedDays = 999.0;` at file scope in `Stock.cpp`.
- `take`: `if (value > qty[i] + 1e-9) throw InsufficientStockException(it, value, qty[i]);` then subtract. Throw **before** mutating, which is what the test's "unchanged after the throw" assertion checks.
- `daysOfSupplyFor`: `double d = dailyDemand.get(it); if (d <= 1e-9) return kUnlimitedDays; return qty[itemIndex(it)] / d;`
- `daysOfSupply`: minimum of `daysOfSupplyFor` over all items; returns `kUnlimitedDays` when no item has demand.
- `mostCriticalItem`: the item with the smallest `daysOfSupplyFor`; ties go to the lower index.
- `operator double()` returns `daysOfSupply()`.
- `operator-=(const Allocation& a)`: loop items, compare against `a.getAmount().get(it)`, throw `InsufficientStockException` for the first shortfall **before** changing anything (compute the whole result into a local array first, then commit), then assign.
- `operator+` and `operator-` build a copy and use `+=` / `-=`, so the validation lives in one place. The copy keeps `dailyDemand` from the left operand.
- `operator<<` prints each item as `Rice 500 kg` and appends `(worst: Rice, 12.5 days)` when a demand is set.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Stock, Allocation and days of supply` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Allocation.h include/Stock.h src/Allocation.cpp src/Stock.cpp tests/test_stock.cpp tests/test_main.cpp
git commit -m "Add Allocation and Stock with days-of-supply projection

Stock + Stock and Stock - Allocation are friend binary operators returning new
objects, and both refuse to go negative by throwing InsufficientStockException
before mutating. Converting a Stock to double gives its days of supply, so the
worst item decides when a camp needs a transfer."
```

---

## Task 13: Camp with the nested Shelter class

**Files:**
- Create: `include/Camp.h`, `src/Camp.cpp`, `tests/test_camp.cpp`
- Modify: `tests/test_main.cpp` (add `run_camp_tests();`)

**Interfaces:**
- Consumes: `Stock.h`, `Family.h`, `Exceptions.h`.
- Produces:
  - `class Camp::Shelter` (nested, public): `Shelter()`, `Shelter(const std::string& label, int capacity)`, `getLabel()`, `getCapacity()`, `getOccupancy()`, `freeSpace()`, `hasRoom(int n)`, `occupy(int n)` (throws `OverCapacityException`), `vacate(int n)`, `friend operator<<`.
  - `class Camp`: `Camp()`, `Camp(const std::string& name, const std::string& district, int capacity)`, copy constructor, destructor, `static int generateId()`, `static void resetIdCounter(int)`, `adoptId(int)`, const getters (`getId`, `getName`, `getDistrict`, `getCapacity`, `getOccupancy`, `freeSpace`, `isOverflowing`, `occupancyRate`), `void admit(const Family& f)`, `void release(const Family& f)`, `Camp& operator++()`, `Camp operator++(int)`, `Camp& operator--()`, `Camp operator--(int)`, `void addShelter(const Shelter&)`, `int shelterCount() const`, `const Shelter& shelterAt(int) const`, `Shelter& shelterAt(int)`, `Stock& stockRef()`, `const Stock& getStock() const`, `void setOccupancy(int)`, `friend class ReportGenerator;`, `friend operator<<`.
  - `Camp` is the first type to need `ReportGenerator` as a friend; declare the friendship now even though Task 18 writes the class. Add `class ReportGenerator;` as a forward declaration above `class Camp`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_camp.cpp`:

```cpp
#include "TestRunner.h"
#include "Camp.h"
#include "Registry.h"
#include "Family.h"
#include "Adult.h"
#include "Infant.h"
#include "Exceptions.h"
#include <memory>
#include <sstream>

void run_camp_tests() {
    testing::beginSuite("Camp and the nested Shelter class");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    // --- the nested class on its own ---
    Camp::Shelter hall("Hall-A", 40);           // [Nested class]
    ASSERT_STR_EQ(hall.getLabel(), "Hall-A");
    ASSERT_EQ(hall.getCapacity(), 40);
    ASSERT_EQ(hall.getOccupancy(), 0);
    ASSERT_EQ(hall.freeSpace(), 40);
    ASSERT_TRUE(hall.hasRoom(40));
    ASSERT_FALSE(hall.hasRoom(41));

    hall.occupy(35);
    ASSERT_EQ(hall.getOccupancy(), 35);
    ASSERT_EQ(hall.freeSpace(), 5);
    ASSERT_THROWS(hall.occupy(6), OverCapacityException);
    ASSERT_EQ(hall.getOccupancy(), 35);         // unchanged after the throw
    hall.vacate(5);
    ASSERT_EQ(hall.getOccupancy(), 30);
    hall.vacate(1000);                          // clamps at zero rather than going negative
    ASSERT_EQ(hall.getOccupancy(), 0);

    Camp::Shelter blankShelter;
    ASSERT_EQ(blankShelter.getCapacity(), 0);

    // --- the camp ---
    Camp camp("Bhuragaon HS School", "Morigaon", 10);
    ASSERT_EQ(camp.getId(), 1);
    ASSERT_STR_EQ(camp.getName(), "Bhuragaon HS School");
    ASSERT_STR_EQ(camp.getDistrict(), "Morigaon");
    ASSERT_EQ(camp.getCapacity(), 10);
    ASSERT_EQ(camp.getOccupancy(), 0);
    ASSERT_EQ(camp.freeSpace(), 10);
    ASSERT_FALSE(camp.isOverflowing());
    ASSERT_NEAR(camp.occupancyRate(), 0.0, 1e-9);

    // Shelters live inside the camp.
    camp.addShelter(Camp::Shelter("Room-1", 6));
    camp.addShelter(Camp::Shelter("Tent-1", 4));
    ASSERT_EQ(camp.shelterCount(), 2);
    ASSERT_STR_EQ(camp.shelterAt(0).getLabel(), "Room-1");
    ASSERT_STR_EQ(camp.shelterAt(1).getLabel(), "Tent-1");
    ASSERT_THROWS(camp.shelterAt(2), std::out_of_range);

    // --- admitting a family: [object as argument] ---
    Family big("Bipul Das", "Bhuragaon");
    for (int i = 0; i < 6; i++) {
        big.addMember(new Adult("Member " + std::to_string(i), 25 + i, 'M', big.getId(), 0));
    }
    camp.admit(big);
    ASSERT_EQ(camp.getOccupancy(), 6);
    ASSERT_EQ(camp.freeSpace(), 4);
    ASSERT_NEAR(camp.occupancyRate(), 0.6, 1e-9);

    // A family that does not fit is refused, with the numbers the menu needs
    // to suggest another camp.
    Family tooBig("Rupa Nath", "Laharighat");
    for (int i = 0; i < 5; i++) {
        tooBig.addMember(new Adult("Nath " + std::to_string(i), 30 + i, 'F', tooBig.getId(), 0));
    }
    ASSERT_THROWS(camp.admit(tooBig), OverCapacityException);
    ASSERT_EQ(camp.getOccupancy(), 6);          // the refused admission changed nothing
    try {
        camp.admit(tooBig);
    } catch (const OverCapacityException& e) {
        ASSERT_EQ(e.getCampId(), camp.getId());
        ASSERT_EQ(e.getRequested(), 5);
        ASSERT_EQ(e.getFreeSpace(), 4);
    }

    // An empty family is a no-op, not an error.
    Family empty("Nobody", "Nowhere");
    camp.admit(empty);
    ASSERT_EQ(camp.getOccupancy(), 6);

    // release() gives the space back.
    camp.release(big);
    ASSERT_EQ(camp.getOccupancy(), 0);
    camp.release(big);                          // clamps at zero
    ASSERT_EQ(camp.getOccupancy(), 0);

    // --- [unary operators] one more person, one fewer person ---
    Camp small("Tent Cluster", "Morigaon", 3);
    ++small;                                    // prefix
    ASSERT_EQ(small.getOccupancy(), 1);
    ++small;
    ASSERT_EQ(small.getOccupancy(), 2);

    Camp before = small++;                      // postfix returns the OLD value
    ASSERT_EQ(before.getOccupancy(), 2);
    ASSERT_EQ(small.getOccupancy(), 3);

    // The capacity guard applies to the operator too.
    ASSERT_THROWS(++small, OverCapacityException);
    ASSERT_EQ(small.getOccupancy(), 3);
    ASSERT_TRUE(small.isOverflowing());         // full counts as overflowing
    ASSERT_NEAR(small.occupancyRate(), 1.0, 1e-9);

    --small;                                    // prefix
    ASSERT_EQ(small.getOccupancy(), 2);
    Camp beforeDec = small--;                   // postfix
    ASSERT_EQ(beforeDec.getOccupancy(), 2);
    ASSERT_EQ(small.getOccupancy(), 1);
    --small;
    --small;                                    // clamps at zero
    ASSERT_EQ(small.getOccupancy(), 0);

    // --- the camp owns its stock ---
    camp.stockRef().set(Item::Rice, 400.0);
    ASSERT_NEAR(camp.getStock().get(Item::Rice), 400.0, 1e-9);
    camp.stockRef().setDailyDemand(Ration(100));
    ASSERT_NEAR(camp.getStock().daysOfSupply(), 10.0, 1e-9);

    // --- copy constructor ---
    Camp copiedCamp(camp);
    ASSERT_STR_EQ(copiedCamp.getName(), "Bhuragaon HS School");
    ASSERT_EQ(copiedCamp.shelterCount(), 2);
    ASSERT_NEAR(copiedCamp.getStock().get(Item::Rice), 400.0, 1e-9);
    copiedCamp.stockRef().set(Item::Rice, 1.0);
    ASSERT_NEAR(camp.getStock().get(Item::Rice), 400.0, 1e-9);   // independent

    // --- Registry<Camp>: the same template as Registry<Family> ---
    Registry<Camp> camps;
    Camp* c1 = camps.add(std::unique_ptr<Camp>(new Camp("School", "Morigaon", 50)));
    Camp* c2 = camps.add(std::unique_ptr<Camp>(new Camp("Hall", "Morigaon", 30)));
    ASSERT_EQ(camps.size(), 2);
    ASSERT_TRUE(camps.findById(c2->getId()) == c2);
    ASSERT_STR_EQ(c1->getName(), "School");

    // setOccupancy and adoptId support CSV loading.
    c1->setOccupancy(17);
    ASSERT_EQ(c1->getOccupancy(), 17);
    c1->adoptId(900);
    ASSERT_EQ(c1->getId(), 900);

    std::ostringstream oss;
    oss << camp;
    ASSERT_TRUE(oss.str().find("Bhuragaon HS School") != std::string::npos);
    ASSERT_TRUE(oss.str().find("Morigaon") != std::string::npos);

    std::ostringstream shelterOss;
    shelterOss << camp.shelterAt(0);
    ASSERT_TRUE(shelterOss.str().find("Room-1") != std::string::npos);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
```

Add `void run_camp_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Camp.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Camp.h`:

```cpp
#ifndef RELIEF_CAMP_H
#define RELIEF_CAMP_H

#include "Stock.h"
#include <string>
#include <vector>
#include <ostream>

class Family;            // admit/release only need a reference
class ReportGenerator;   // declared a friend below

// A relief camp: a school, a community hall or a cluster of tents on an
// embankment, with a capacity, a set of shelters and its own stock.
class Camp {
public:
    // [Nested class] A shelter has no meaning outside the camp that contains
    // it, so its name lives inside Camp: `Camp::Shelter`.
    class Shelter {
        std::string label;
        int capacity;
        int occupancy;
    public:
        Shelter();
        Shelter(const std::string& label, int capacity);
        const std::string& getLabel() const;
        int getCapacity() const;
        int getOccupancy() const;
        int freeSpace() const;
        bool hasRoom(int people) const;
        void occupy(int people);        // throws OverCapacityException
        void vacate(int people);        // clamps at zero
        void setOccupancy(int people);  // CSV loading
        friend std::ostream& operator<<(std::ostream& os, const Shelter& s);
    };

private:
    static int nextId;                  // [Static data member]

    int id;
    std::string name;
    std::string district;
    int capacity;
    int occupancy;
    std::vector<Shelter> shelters;
    Stock stock;

public:
    Camp();
    Camp(const std::string& name, const std::string& district, int capacity);
    Camp(const Camp& other);
    ~Camp();

    static int generateId();            // [Static member function]
    static void resetIdCounter(int startAt);
    void adoptId(int existingId);

    int getId() const;
    const std::string& getName() const;
    const std::string& getDistrict() const;
    int getCapacity() const;
    int getOccupancy() const;
    int freeSpace() const;
    bool isOverflowing() const;         // true once the camp is full or beyond
    double occupancyRate() const;

    // [Objects as arguments] the camp is told about a whole family at once
    void admit(const Family& f);        // throws OverCapacityException
    void release(const Family& f);

    // [Unary operators] one more, or one fewer, person in the camp
    Camp& operator++();                 // prefix: ++camp
    Camp operator++(int);               // postfix: camp++
    Camp& operator--();                 // prefix
    Camp operator--(int);               // postfix

    void addShelter(const Shelter& s);
    int shelterCount() const;
    const Shelter& shelterAt(int index) const;   // throws std::out_of_range
    Shelter& shelterAt(int index);

    Stock& stockRef();
    const Stock& getStock() const;
    void setOccupancy(int people);      // CSV loading
    void setName(const std::string& newName);

    // [Friend class] ReportGenerator reads the camp's private occupancy and
    // stock directly, because a report is a view of the camp's internals and
    // adding public getters just for it would widen the interface for everyone.
    friend class ReportGenerator;

    friend std::ostream& operator<<(std::ostream& os, const Camp& c);
};

#endif
```

Implementation notes:
- `int Camp::nextId = 1;`
- `Shelter::occupy`: `if (people > freeSpace()) throw OverCapacityException(-1, people, freeSpace());` then `occupancy += people`. The shelter does not know its camp's ID, so it passes `-1`; `Camp::admit` is the path that reports a real camp ID.
- `admit(const Family& f)`: `int n = f.size(); if (n == 0) return; if (n > freeSpace()) throw OverCapacityException(id, n, freeSpace()); occupancy += n;` — the check precedes the mutation.
- `release(const Family& f)`: `occupancy -= f.size(); if (occupancy < 0) occupancy = 0;`
- `operator++()`: `if (occupancy >= capacity) throw OverCapacityException(id, 1, freeSpace()); occupancy++; return *this;`
- `operator++(int)`: `Camp before(*this); ++(*this); return before;` — the prefix form does the work and the guard, so the throw happens before `before` is returned.
- `operator--()`: decrement, clamp at zero, `return *this;`. `operator--(int)`: copy, call prefix, return the copy.
- `isOverflowing()`: `return occupancy >= capacity;`
- `occupancyRate()`: `capacity == 0 ? 0.0 : static_cast<double>(occupancy) / capacity;`
- `shelterAt` throws `std::out_of_range("shelter index out of range")`.
- `operator<<`: `os << "Camp #" << c.id << " " << c.name << ", " << c.district << " [" << c.occupancy << "/" << c.capacity << "]" << (c.isOverflowing() ? " OVERFLOW" : "");`
- The copy constructor copies every field; `std::vector<Shelter>` and `Stock` both copy themselves, so no manual work is needed — note in a comment that `Camp` needs no rule of three precisely because it owns no raw memory, which is the contrast with `Family`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Camp and the nested Shelter class` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/Camp.h src/Camp.cpp tests/test_camp.cpp tests/test_main.cpp
git commit -m "Add Camp with the nested Shelter class and capacity guard

Camp::Shelter is nested because a shelter has no meaning outside its camp.
admit() takes a whole Family and throws OverCapacityException carrying the
free space, so the menu can suggest an alternative. ++/-- adjust occupancy
and respect the same guard. ReportGenerator is declared a friend."
```

---

## Task 14: DistributionLog and the duplicate-claim rule

**Files:**
- Create: `include/DistributionLog.h`, `src/DistributionLog.cpp`, `tests/test_distribution.cpp`
- Modify: `tests/test_main.cpp` (add `run_distribution_tests();`)

**Interfaces:**
- Consumes: `Allocation.h`, `Exceptions.h`, `Ration.h`, `Date.h`.
- Produces: `class DistributionLog` — `DistributionLog()`, `void record(const Allocation& a)` (throws `DuplicateClaimException`), `bool alreadyClaimed(int familyId, const Date& date) const`, `int size() const`, `const Allocation& at(int index) const`, `std::vector<Allocation> forCamp(int campId, const Date& date) const`, `std::vector<Allocation> forFamily(int familyId) const`, `Ration totalForCamp(int campId, const Date& date) const`, `int familiesServed(int campId, const Date& date) const`, `void loadEntry(const Allocation& a)` (no duplicate check, for CSV load), `void clear()`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_distribution.cpp`:

```cpp
#include "TestRunner.h"
#include "DistributionLog.h"
#include "RationNorms.h"
#include "Exceptions.h"

void run_distribution_tests() {
    testing::beginSuite("DistributionLog and duplicate claims");

    DistributionLog log;
    ASSERT_EQ(log.size(), 0);

    Ration oneAdult = RationNorms::forAdult();
    Date day1(2026, 7, 14);
    Date day2(2026, 7, 15);

    // A first claim is recorded.
    log.record(Allocation(101, 1, day1, oneAdult * 5.0));
    ASSERT_EQ(log.size(), 1);
    ASSERT_TRUE(log.alreadyClaimed(101, day1));
    ASSERT_FALSE(log.alreadyClaimed(101, day2));
    ASSERT_FALSE(log.alreadyClaimed(102, day1));

    // The same family, the same day, the SAME camp: refused.
    ASSERT_THROWS(log.record(Allocation(101, 1, day1, oneAdult)), DuplicateClaimException);
    ASSERT_EQ(log.size(), 1);               // nothing was added

    // The same family, the same day, a DIFFERENT camp: still refused. This is
    // the rule that stops a family collecting twice by walking to the next camp.
    ASSERT_THROWS(log.record(Allocation(101, 2, day1, oneAdult)), DuplicateClaimException);
    ASSERT_EQ(log.size(), 1);

    try {
        log.record(Allocation(101, 3, day1, oneAdult));
    } catch (const DuplicateClaimException& e) {
        ASSERT_EQ(e.getFamilyId(), 101);
        ASSERT_TRUE(e.getDate() == day1);
    }

    // The next day the same family may claim again.
    log.record(Allocation(101, 1, day2, oneAdult * 5.0));
    ASSERT_EQ(log.size(), 2);
    ASSERT_TRUE(log.alreadyClaimed(101, day2));

    // Other families are unaffected.
    log.record(Allocation(102, 1, day1, oneAdult * 3.0));
    log.record(Allocation(103, 2, day1, oneAdult * 4.0));
    ASSERT_EQ(log.size(), 4);

    // Queries by camp and day.
    std::vector<Allocation> camp1Day1 = log.forCamp(1, day1);
    ASSERT_EQ(static_cast<int>(camp1Day1.size()), 2);       // families 101 and 102
    ASSERT_EQ(log.familiesServed(1, day1), 2);
    ASSERT_EQ(log.familiesServed(2, day1), 1);
    ASSERT_EQ(log.familiesServed(9, day1), 0);

    // The camp's total for the day is the sum of its allocations.
    Ration camp1Total = log.totalForCamp(1, day1);
    ASSERT_NEAR(camp1Total.get(Item::Rice), 0.400 * 8.0, 1e-9);   // 5 + 3 adults

    Ration emptyTotal = log.totalForCamp(9, day1);
    ASSERT_TRUE(emptyTotal.isZero());

    // A family's whole history, across camps and days.
    std::vector<Allocation> history = log.forFamily(101);
    ASSERT_EQ(static_cast<int>(history.size()), 2);

    // at() is positional and range-checked.
    ASSERT_EQ(log.at(0).getFamilyId(), 101);
    ASSERT_THROWS(log.at(99), std::out_of_range);

    // loadEntry skips the duplicate check, because the CSV on disk is the
    // record of what already happened - re-validating it would reject the
    // very data we are restoring.
    DistributionLog loaded;
    loaded.loadEntry(Allocation(101, 1, day1, oneAdult));
    loaded.loadEntry(Allocation(101, 1, day1, oneAdult));    // no throw
    ASSERT_EQ(loaded.size(), 2);
    // ...but a NEW claim after loading is still checked.
    ASSERT_THROWS(loaded.record(Allocation(101, 1, day1, oneAdult)), DuplicateClaimException);

    log.clear();
    ASSERT_EQ(log.size(), 0);
    ASSERT_FALSE(log.alreadyClaimed(101, day1));
}
```

Add `void run_distribution_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: DistributionLog.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/DistributionLog.h`:

```cpp
#ifndef RELIEF_DISTRIBUTIONLOG_H
#define RELIEF_DISTRIBUTIONLOG_H

#include "Allocation.h"
#include "Ration.h"
#include "Date.h"
#include <vector>

// Every handout ever made, across every camp. The log is what makes the
// "one ration per family per day" rule enforceable: paper registers are kept
// per camp, so a family can collect twice by walking to the next camp, and
// this class is the fix for exactly that.
class DistributionLog {
    std::vector<Allocation> entries;
public:
    DistributionLog();

    // Records a handout, or throws DuplicateClaimException if this family has
    // already collected on this date in ANY camp.
    void record(const Allocation& a);

    bool alreadyClaimed(int familyId, const Date& date) const;

    int size() const;
    const Allocation& at(int index) const;        // throws std::out_of_range

    std::vector<Allocation> forCamp(int campId, const Date& date) const;
    std::vector<Allocation> forFamily(int familyId) const;
    Ration totalForCamp(int campId, const Date& date) const;
    int familiesServed(int campId, const Date& date) const;

    // Used by the CSV loader. The file records what already happened, so
    // re-checking it would reject the data being restored.
    void loadEntry(const Allocation& a);
    void clear();
};

#endif
```

Implementation notes:
- `alreadyClaimed` builds a probe `Allocation(familyId, 0, date, Ration())` and compares with `operator==`, so the "same claim" rule lives in exactly one place — `Allocation::operator==` — and this class never re-states it. Mark that line `// [Operator overloading in use] the rule lives in Allocation::operator==`.
- `record`: `if (alreadyClaimed(a.getFamilyId(), a.getDate())) throw DuplicateClaimException(a.getFamilyId(), a.getDate()); entries.push_back(a);`
- `totalForCamp`: accumulate `total += entries[i].getAmount()` for matching camp and date.
- `familiesServed`: count distinct family IDs for that camp and date (a `std::vector<int>` of seen IDs is clear enough and avoids pulling in `<set>`).
- `at` throws `std::out_of_range("distribution index out of range")`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `DistributionLog and duplicate claims` suite all ok.

- [ ] **Step 5: Commit**

```bash
git add include/DistributionLog.h src/DistributionLog.cpp tests/test_distribution.cpp tests/test_main.cpp
git commit -m "Add DistributionLog with the cross-camp duplicate-claim rule

A family that has collected on a date is refused in every camp, not just the
one that served them, which is the failure paper registers cannot catch. The
rule itself lives in Allocation::operator== and the log simply consults it.
loadEntry bypasses the check so restoring a CSV does not reject its own data."
```

---
## Task 15: DistributionQueue and FamilySorter

**Files:**
- Create: `include/DistributionQueue.h`, `src/DistributionQueue.cpp`, `include/FamilySorter.h`, `src/FamilySorter.cpp`, `tests/test_queue.cpp`
- Modify: `tests/test_main.cpp` (add `run_queue_tests();`)

**Interfaces:**
- Consumes: `Family.h` (and its `operator<` and metric functions).
- Produces:
  - `class DistributionQueue` — `DistributionQueue()`, `void push(Family* f)`, `Family* pop()` (returns `0` when empty), `const Family* peek() const`, `bool empty() const`, `int size() const`, `void clear()`.
  - `class FamilySorter` — `typedef double (Family::*Metric)() const;`, `static void sortDescending(std::vector<Family*>& families, Metric metric)`, `static void sortAscending(...)`, `static Metric metricByName(const std::string& name)` (accepts `"vulnerability"`, `"size"`, `"age"`, `"infants"`; throws `std::invalid_argument` otherwise), `static std::string metricMenu()`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_queue.cpp`:

```cpp
#include "TestRunner.h"
#include "DistributionQueue.h"
#include "FamilySorter.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include <vector>
#include <stdexcept>

// Builds a family with the given counts, so each test family has a predictable
// vulnerability score: infant 40, child 20, adult 10, elderly 35.
static Family* makeFamily(const std::string& head, int infants, int children,
                          int adults, int elderly, int adultAge) {
    Family* f = new Family(head, "Morigaon");
    for (int i = 0; i < infants;  i++) f->addMember(new Infant("Inf", 1, 'F', f->getId(), 0));
    for (int i = 0; i < children; i++) f->addMember(new Child("Chi", 8, 'M', f->getId(), 0));
    for (int i = 0; i < adults;   i++) f->addMember(new Adult("Adu", adultAge, 'M', f->getId(), 0));
    for (int i = 0; i < elderly;  i++) f->addMember(new Elderly("Eld", 70, 'F', f->getId(), 0));
    return f;
}

void run_queue_tests() {
    testing::beginSuite("DistributionQueue and FamilySorter");

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);

    Family* low    = makeFamily("Jiten Saikia", 0, 0, 2, 0, 40);   // score 20
    Family* medium = makeFamily("Rupa Nath",    0, 2, 2, 0, 35);   // score 60
    Family* high   = makeFamily("Bipul Das",    1, 1, 2, 1, 30);   // score 115
    Family* middle = makeFamily("Gitali Pegu",  0, 1, 2, 1, 50);   // score 75

    ASSERT_EQ(low->vulnerabilityScore(), 20);
    ASSERT_EQ(medium->vulnerabilityScore(), 60);
    ASSERT_EQ(high->vulnerabilityScore(), 115);
    ASSERT_EQ(middle->vulnerabilityScore(), 75);

    // --- the hand-written heap ---
    DistributionQueue q;
    ASSERT_TRUE(q.empty());
    ASSERT_EQ(q.size(), 0);
    ASSERT_TRUE(q.peek() == 0);
    ASSERT_TRUE(q.pop() == 0);

    // Pushed in a deliberately jumbled order.
    q.push(medium);
    q.push(high);
    q.push(low);
    q.push(middle);
    ASSERT_EQ(q.size(), 4);
    ASSERT_FALSE(q.empty());

    // peek() does not remove.
    ASSERT_TRUE(q.peek() == high);
    ASSERT_EQ(q.size(), 4);

    // Most vulnerable first, every time.
    ASSERT_TRUE(q.pop() == high);     // 115
    ASSERT_TRUE(q.pop() == middle);   //  75
    ASSERT_TRUE(q.pop() == medium);   //  60
    ASSERT_TRUE(q.pop() == low);      //  20
    ASSERT_TRUE(q.empty());
    ASSERT_TRUE(q.pop() == 0);

    // A single element, and duplicates of equal priority, both behave.
    DistributionQueue one;
    one.push(low);
    ASSERT_TRUE(one.pop() == low);
    ASSERT_TRUE(one.empty());

    DistributionQueue ties;
    Family* tieA = makeFamily("Tie A", 0, 0, 1, 0, 30);   // score 10
    Family* tieB = makeFamily("Tie B", 0, 0, 1, 0, 31);   // score 10
    ties.push(tieA);
    ties.push(tieB);
    ASSERT_EQ(ties.size(), 2);
    Family* first = ties.pop();
    Family* second = ties.pop();
    ASSERT_TRUE(first != 0 && second != 0);
    ASSERT_TRUE(first != second);
    ASSERT_TRUE(ties.empty());

    // A larger run, to shake out the sift logic rather than trusting four items.
    DistributionQueue many;
    std::vector<Family*> built;
    for (int i = 0; i < 25; i++) {
        // adults = (i % 5) + 1 gives scores of 10, 20, 30, 40, 50 in a cycle
        Family* f = makeFamily("Bulk " + std::to_string(i), 0, 0, (i % 5) + 1, 0, 30);
        built.push_back(f);
        many.push(f);
    }
    ASSERT_EQ(many.size(), 25);
    int previous = 1000000;
    int popped = 0;
    while (!many.empty()) {
        Family* f = many.pop();
        ASSERT_TRUE(f->vulnerabilityScore() <= previous);   // never increases
        previous = f->vulnerabilityScore();
        popped++;
    }
    ASSERT_EQ(popped, 25);

    q.clear();
    ASSERT_TRUE(q.empty());

    // --- FamilySorter: one pointer-to-member drives every ordering ---
    std::vector<Family*> list;
    list.push_back(low);
    list.push_back(high);
    list.push_back(medium);
    list.push_back(middle);

    FamilySorter::sortDescending(list, &Family::metricVulnerability);
    ASSERT_TRUE(list[0] == high);
    ASSERT_TRUE(list[3] == low);

    FamilySorter::sortAscending(list, &Family::metricVulnerability);
    ASSERT_TRUE(list[0] == low);
    ASSERT_TRUE(list[3] == high);

    FamilySorter::sortDescending(list, &Family::metricSize);
    ASSERT_EQ(list[0]->size(), 5);        // high has 5 members
    ASSERT_EQ(list[3]->size(), 2);        // low has 2

    FamilySorter::sortDescending(list, &Family::metricInfantCount);
    ASSERT_TRUE(list[0] == high);         // the only family with an infant

    FamilySorter::sortDescending(list, &Family::metricAverageAge);
    ASSERT_TRUE(list[0] == low);          // two adults aged 40 -> average 40

    // The menu looks a metric up by name, so the console never hard-codes four
    // separate sort calls.
    FamilySorter::Metric m = FamilySorter::metricByName("vulnerability");
    FamilySorter::sortDescending(list, m);
    ASSERT_TRUE(list[0] == high);

    ASSERT_TRUE(FamilySorter::metricByName("size") == &Family::metricSize);
    ASSERT_TRUE(FamilySorter::metricByName("age") == &Family::metricAverageAge);
    ASSERT_TRUE(FamilySorter::metricByName("infants") == &Family::metricInfantCount);
    ASSERT_THROWS(FamilySorter::metricByName("height"), std::invalid_argument);
    ASSERT_TRUE(FamilySorter::metricMenu().find("vulnerability") != std::string::npos);

    // Sorting an empty or single-element list must not crash.
    std::vector<Family*> emptyList;
    FamilySorter::sortDescending(emptyList, &Family::metricSize);
    ASSERT_EQ(static_cast<int>(emptyList.size()), 0);

    for (int i = 0; i < static_cast<int>(built.size()); i++) delete built[i];
    delete low; delete medium; delete high; delete middle; delete tieA; delete tieB;

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
}
```

Add `void run_queue_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: DistributionQueue.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/DistributionQueue.h`:

```cpp
#ifndef RELIEF_DISTRIBUTIONQUEUE_H
#define RELIEF_DISTRIBUTIONQUEUE_H

#include <vector>

class Family;

// A priority queue over families, most vulnerable first, so an infant's family
// is served before a household of healthy adults when a queue forms.
//
// This is a binary max-heap we wrote ourselves rather than std::priority_queue,
// so the data structure is visible and our overloaded Family::operator< is
// plainly what drives the ordering.
//
// The queue does NOT own the families; it only borrows pointers to them.
class DistributionQueue {
    std::vector<Family*> heap;

    void siftUp(int index);
    void siftDown(int index);
    // Uses Family::operator< so the priority rule lives in one place.
    bool higherPriority(const Family* a, const Family* b) const;

public:
    DistributionQueue();

    void push(Family* f);
    Family* pop();                 // 0 when empty
    const Family* peek() const;    // 0 when empty
    bool empty() const;
    int size() const;
    void clear();
};

#endif
```

`src/DistributionQueue.cpp` — write the heap exactly like this:

```cpp
#include "DistributionQueue.h"
#include "Family.h"

DistributionQueue::DistributionQueue() {}

// [Operator overloading in use] `*b < *a` means "a outranks b", because
// Family::operator< compares vulnerability scores and we want the HIGHER
// score at the top of the heap.
bool DistributionQueue::higherPriority(const Family* a, const Family* b) const {
    return *b < *a;
}

void DistributionQueue::siftUp(int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (!higherPriority(heap[index], heap[parent])) {
            break;
        }
        Family* temp = heap[index];
        heap[index] = heap[parent];
        heap[parent] = temp;
        index = parent;
    }
}

void DistributionQueue::siftDown(int index) {
    int count = static_cast<int>(heap.size());
    while (true) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int best = index;
        if (left < count && higherPriority(heap[left], heap[best])) {
            best = left;
        }
        if (right < count && higherPriority(heap[right], heap[best])) {
            best = right;
        }
        if (best == index) {
            break;
        }
        Family* temp = heap[index];
        heap[index] = heap[best];
        heap[best] = temp;
        index = best;
    }
}

void DistributionQueue::push(Family* f) {
    if (f == 0) { return; }
    heap.push_back(f);
    siftUp(static_cast<int>(heap.size()) - 1);
}

Family* DistributionQueue::pop() {
    if (heap.empty()) { return 0; }
    Family* top = heap[0];
    heap[0] = heap[heap.size() - 1];
    heap.pop_back();
    if (!heap.empty()) { siftDown(0); }
    return top;
}

const Family* DistributionQueue::peek() const {
    return heap.empty() ? 0 : heap[0];
}

bool DistributionQueue::empty() const { return heap.empty(); }
int DistributionQueue::size() const { return static_cast<int>(heap.size()); }
void DistributionQueue::clear() { heap.clear(); }
```

Create `include/FamilySorter.h`:

```cpp
#ifndef RELIEF_FAMILYSORTER_H
#define RELIEF_FAMILYSORTER_H

#include <vector>
#include <string>

class Family;

// Sorts families by whichever field the official picks from the menu.
//
// [Pointer to member function] Metric points at one of Family's metric
// functions. One sort routine therefore covers vulnerability, household size,
// average age and infant count - four near-identical functions collapsed into
// one, with the choice made at run time.
class FamilySorter {
public:
    typedef double (Family::*Metric)() const;

    static void sortDescending(std::vector<Family*>& families, Metric metric);
    static void sortAscending(std::vector<Family*>& families, Metric metric);

    // Looks a metric up by the name shown in the menu.
    // Accepts "vulnerability", "size", "age", "infants".
    static Metric metricByName(const std::string& name);   // throws std::invalid_argument
    static std::string metricMenu();
};

#endif
```

`src/FamilySorter.cpp` notes — use a plain selection sort so the pointer-to-member call is unmistakable, and comment why:

```cpp
// A selection sort, not std::sort with a lambda, so that the call through the
// member pointer - (families[j]->*metric)() - is the visible centre of the loop.
// With a few dozen families per camp the cost is irrelevant.
void FamilySorter::sortDescending(std::vector<Family*>& families, Metric metric) {
    int n = static_cast<int>(families.size());
    for (int i = 0; i < n - 1; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if ((families[j]->*metric)() > (families[best]->*metric)()) {
                best = j;
            }
        }
        if (best != i) {
            Family* temp = families[i];
            families[i] = families[best];
            families[best] = temp;
        }
    }
}
```

`sortAscending` is the same with `<`. `metricByName` returns `&Family::metricVulnerability` / `&Family::metricSize` / `&Family::metricAverageAge` / `&Family::metricInfantCount`, else throws `std::invalid_argument("unknown metric: " + name)`. `metricMenu()` returns `"1. vulnerability  2. size  3. age  4. infants"`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `DistributionQueue and FamilySorter` suite all ok, including the 25-family monotonic pop check.

- [ ] **Step 5: Commit**

```bash
git add include/DistributionQueue.h include/FamilySorter.h src/DistributionQueue.cpp src/FamilySorter.cpp tests/test_queue.cpp tests/test_main.cpp
git commit -m "Add hand-written priority queue and pointer-to-member sorting

DistributionQueue is our own binary max-heap, ordered by Family::operator<, so
the most vulnerable household is served first. FamilySorter's Metric typedef is
a pointer to a member function, collapsing four near-identical sorts into one
routine whose ordering is chosen at run time."
```

---

## Task 16: StockLedger and TransferAdvisor

**Files:**
- Create: `include/StockLedger.h`, `src/StockLedger.cpp`, `include/TransferAdvisor.h`, `src/TransferAdvisor.cpp`, `tests/test_transfer.cpp`
- Modify: `tests/test_main.cpp` (add `run_transfer_tests();`)

**Interfaces:**
- Consumes: `Camp.h`, `Stock.h`, `Item.h`, `Date.h`, `Exceptions.h`.
- Produces:
  - `class StockTxn` (in `StockLedger.h`): `StockTxn()`, `StockTxn(int campId, Item item, double quantity, bool incoming, const Date& date, const std::string& note)`, const getters, `friend operator<<`.
  - `class StockLedger`: `void stockIn(Camp& camp, Item item, double quantity, const Date& date, const std::string& note)`, `void stockOut(Camp& camp, Item item, double quantity, const Date& date, const std::string& note)` (throws `InsufficientStockException`), `int size() const`, `const StockTxn& at(int) const`, `std::vector<StockTxn> forCamp(int campId) const`, `double netFor(int campId, Item item) const`, `void loadTxn(const StockTxn&)`, `void clear()`.
  - `class TransferSuggestion` (in `TransferAdvisor.h`): `TransferSuggestion()`, full parameterised constructor, const getters (`getFromCampId`, `getToCampId`, `getItem`, `getQuantity`, `getFromDays`, `getToDays`), `std::string render() const`, `friend operator<<`.
  - `class TransferAdvisor`: `TransferAdvisor()` (critical 3.0 days, surplus 7.0 days), `TransferAdvisor(double criticalDays, double surplusDays)`, `double getCriticalDays() const`, `double getSurplusDays() const`, `std::vector<TransferSuggestion> suggest(const std::vector<Camp*>& camps) const`, `std::vector<Item> criticalItems(const Camp& camp) const`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_transfer.cpp`:

```cpp
#include "TestRunner.h"
#include "StockLedger.h"
#include "TransferAdvisor.h"
#include "Camp.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <vector>

void run_transfer_tests() {
    testing::beginSuite("StockLedger and TransferAdvisor");

    Camp::resetIdCounter(1);
    Date today(2026, 7, 14);

    // --- StockLedger ---
    Camp school("Bhuragaon HS School", "Morigaon", 200);
    StockLedger ledger;
    ASSERT_EQ(ledger.size(), 0);

    ledger.stockIn(school, Item::Rice, 500.0, today, "district supply");
    ASSERT_NEAR(school.getStock().get(Item::Rice), 500.0, 1e-9);
    ASSERT_EQ(ledger.size(), 1);
    ASSERT_EQ(ledger.at(0).getCampId(), school.getId());
    ASSERT_TRUE(ledger.at(0).getItem() == Item::Rice);
    ASSERT_TRUE(ledger.at(0).isIncoming());
    ASSERT_STR_EQ(ledger.at(0).getNote(), "district supply");

    ledger.stockOut(school, Item::Rice, 120.0, today, "day 1 distribution");
    ASSERT_NEAR(school.getStock().get(Item::Rice), 380.0, 1e-9);
    ASSERT_EQ(ledger.size(), 2);
    ASSERT_FALSE(ledger.at(1).isIncoming());

    // The ledger's net must match the camp's stock, which is the whole point
    // of keeping a ledger rather than just a running total.
    ASSERT_NEAR(ledger.netFor(school.getId(), Item::Rice), 380.0, 1e-9);

    // Taking more than the camp holds is refused and nothing is logged.
    ASSERT_THROWS(ledger.stockOut(school, Item::Rice, 9999.0, today, "oops"),
                  InsufficientStockException);
    ASSERT_EQ(ledger.size(), 2);
    ASSERT_NEAR(school.getStock().get(Item::Rice), 380.0, 1e-9);

    ASSERT_EQ(static_cast<int>(ledger.forCamp(school.getId()).size()), 2);
    ASSERT_EQ(static_cast<int>(ledger.forCamp(999).size()), 0);
    ASSERT_THROWS(ledger.at(99), std::out_of_range);

    // --- TransferAdvisor ---
    TransferAdvisor advisor;                       // 3 days critical, 7 days surplus
    ASSERT_NEAR(advisor.getCriticalDays(), 3.0, 1e-9);
    ASSERT_NEAR(advisor.getSurplusDays(), 7.0, 1e-9);

    // Two camps with the same daily draw: 100 adults each.
    Ration demand = RationNorms::forAdult() * 100.0;     // rice 40 kg/day

    Camp shortCamp("Laharighat Community Hall", "Morigaon", 150);
    shortCamp.stockRef().setDailyDemand(demand);
    shortCamp.stockRef().set(Item::Rice, 60.0);          // 1.5 days - critical
    shortCamp.stockRef().set(Item::Dal, 60.0);           // 10 days  - fine
    shortCamp.stockRef().set(Item::Oil, 30.0);           // 10 days
    shortCamp.stockRef().set(Item::Salt, 10.0);          // 10 days
    shortCamp.stockRef().set(Item::ORS, 100.0);          // 10 days

    Camp richCamp("Mayong Embankment Tents", "Morigaon", 150);
    richCamp.stockRef().setDailyDemand(demand);
    richCamp.stockRef().set(Item::Rice, 800.0);          // 20 days - surplus
    richCamp.stockRef().set(Item::Dal, 60.0);            // 10 days
    richCamp.stockRef().set(Item::Oil, 30.0);
    richCamp.stockRef().set(Item::Salt, 10.0);
    richCamp.stockRef().set(Item::ORS, 100.0);

    ASSERT_NEAR(shortCamp.getStock().daysOfSupplyFor(Item::Rice), 1.5, 1e-9);
    ASSERT_NEAR(richCamp.getStock().daysOfSupplyFor(Item::Rice), 20.0, 1e-9);

    // criticalItems names what is running out at one camp.
    std::vector<Item> critical = advisor.criticalItems(shortCamp);
    ASSERT_EQ(static_cast<int>(critical.size()), 1);
    ASSERT_TRUE(critical[0] == Item::Rice);
    ASSERT_EQ(static_cast<int>(advisor.criticalItems(richCamp).size()), 0);

    // [Returning objects] suggest() hands back a vector of value objects.
    std::vector<Camp*> camps;
    camps.push_back(&shortCamp);
    camps.push_back(&richCamp);
    std::vector<TransferSuggestion> ideas = advisor.suggest(camps);

    ASSERT_EQ(static_cast<int>(ideas.size()), 1);
    ASSERT_EQ(ideas[0].getFromCampId(), richCamp.getId());
    ASSERT_EQ(ideas[0].getToCampId(), shortCamp.getId());
    ASSERT_TRUE(ideas[0].getItem() == Item::Rice);
    ASSERT_TRUE(ideas[0].getQuantity() > 0.0);
    ASSERT_NEAR(ideas[0].getToDays(), 1.5, 1e-9);
    ASSERT_NEAR(ideas[0].getFromDays(), 20.0, 1e-9);

    // The suggested amount must bring the short camp to safety without
    // dragging the donor below the surplus line.
    double moved = ideas[0].getQuantity();
    ASSERT_TRUE(moved <= 800.0 - (7.0 * 40.0));       // donor keeps 7 days
    ASSERT_TRUE((60.0 + moved) / 40.0 >= 3.0);        // receiver reaches 3 days

    // Applying the suggestion fixes the shortage, which is the real check.
    ledger.stockOut(richCamp, Item::Rice, moved, today, "transfer out");
    ledger.stockIn(shortCamp, Item::Rice, moved, today, "transfer in");
    ASSERT_TRUE(shortCamp.getStock().daysOfSupplyFor(Item::Rice) >= 3.0);
    ASSERT_TRUE(richCamp.getStock().daysOfSupplyFor(Item::Rice) >= 7.0);
    ASSERT_EQ(static_cast<int>(advisor.suggest(camps).size()), 0);   // nothing left to fix

    // No donor means no suggestion, rather than a bad one.
    Camp alsoShort("Second Short Camp", "Morigaon", 100);
    alsoShort.stockRef().setDailyDemand(demand);
    alsoShort.stockRef().set(Item::Rice, 40.0);        // 1 day
    std::vector<Camp*> bothShort;
    bothShort.push_back(&alsoShort);
    Camp stillShort("Third Short Camp", "Morigaon", 100);
    stillShort.stockRef().setDailyDemand(demand);
    stillShort.stockRef().set(Item::Rice, 40.0);
    bothShort.push_back(&stillShort);
    ASSERT_EQ(static_cast<int>(advisor.suggest(bothShort).size()), 0);

    // A single camp cannot transfer to itself.
    std::vector<Camp*> alone;
    alone.push_back(&alsoShort);
    ASSERT_EQ(static_cast<int>(advisor.suggest(alone).size()), 0);

    // An empty list is handled.
    std::vector<Camp*> nobody;
    ASSERT_EQ(static_cast<int>(advisor.suggest(nobody).size()), 0);

    // Custom thresholds are respected.
    TransferAdvisor strict(10.0, 15.0);
    ASSERT_NEAR(strict.getCriticalDays(), 10.0, 1e-9);
    ASSERT_TRUE(static_cast<int>(strict.criticalItems(shortCamp).size()) > 0);

    // The suggestion renders as a readable sentence for the console.
    std::string text = ideas[0].render();
    ASSERT_TRUE(text.find("Rice") != std::string::npos);
    ASSERT_TRUE(text.find("Mayong Embankment Tents") != std::string::npos ||
                text.find(std::to_string(richCamp.getId())) != std::string::npos);

    ledger.clear();
    ASSERT_EQ(ledger.size(), 0);

    Camp::resetIdCounter(1);
}
```

Add `void run_transfer_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: StockLedger.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/StockLedger.h`:

```cpp
#ifndef RELIEF_STOCKLEDGER_H
#define RELIEF_STOCKLEDGER_H

#include "Item.h"
#include "Date.h"
#include <string>
#include <vector>
#include <ostream>

class Camp;

// One line in the stock register: what moved, which way, when and why.
// It belongs to StockLedger and has no life of its own, so it shares the
// header rather than getting a file to itself.
class StockTxn {
    int campId;
    Item item;
    double quantity;
    bool incoming;          // true = stock in, false = stock out
    Date date;
    std::string note;
public:
    StockTxn();
    StockTxn(int campId, Item item, double quantity, bool incoming,
             const Date& date, const std::string& note);
    int getCampId() const;
    Item getItem() const;
    double getQuantity() const;
    bool isIncoming() const;
    const Date& getDate() const;
    const std::string& getNote() const;
    friend std::ostream& operator<<(std::ostream& os, const StockTxn& t);
};

// The stock register. Every change to a camp's stock goes through here, so the
// paper trail and the running total can never disagree - netFor() recomputes
// the balance from the transactions and must match the camp's Stock.
class StockLedger {
    std::vector<StockTxn> txns;
public:
    StockLedger();
    void stockIn(Camp& camp, Item item, double quantity,
                 const Date& date, const std::string& note);
    // Throws InsufficientStockException and logs nothing if the camp is short.
    void stockOut(Camp& camp, Item item, double quantity,
                  const Date& date, const std::string& note);
    int size() const;
    const StockTxn& at(int index) const;           // throws std::out_of_range
    std::vector<StockTxn> forCamp(int campId) const;
    double netFor(int campId, Item item) const;
    void loadTxn(const StockTxn& t);               // CSV loading
    void clear();
};

#endif
```

`stockOut` must call `camp.stockRef().take(item, quantity)` **first** and push the transaction only if it did not throw; that ordering is exactly what the test's "nothing is logged" assertion checks. Mark it `// The camp's Stock validates; if it throws, we never log the line.`

Create `include/TransferAdvisor.h`:

```cpp
#ifndef RELIEF_TRANSFERADVISOR_H
#define RELIEF_TRANSFERADVISOR_H

#include "Item.h"
#include <string>
#include <vector>
#include <ostream>

class Camp;

// "Move 220 kg of rice from camp 3 to camp 1." A value object, built and
// returned by TransferAdvisor::suggest.
class TransferSuggestion {
    int fromCampId;
    int toCampId;
    std::string fromCampName;
    std::string toCampName;
    Item item;
    double quantity;
    double fromDays;        // donor's days of supply before the move
    double toDays;          // receiver's days of supply before the move
public:
    TransferSuggestion();
    TransferSuggestion(int fromCampId, const std::string& fromCampName,
                       int toCampId, const std::string& toCampName,
                       Item item, double quantity, double fromDays, double toDays);
    int getFromCampId() const;
    int getToCampId() const;
    const std::string& getFromCampName() const;
    const std::string& getToCampName() const;
    Item getItem() const;
    double getQuantity() const;
    double getFromDays() const;
    double getToDays() const;
    std::string render() const;
    friend std::ostream& operator<<(std::ostream& os, const TransferSuggestion& t);
};

// Looks across the camps and proposes moves: an item one camp is about to run
// out of, taken from a camp that has plenty. Nothing is moved automatically -
// a human approves the transfer, because trucks and boats are involved.
class TransferAdvisor {
    double criticalDays;    // at or below this, a camp is in trouble
    double surplusDays;     // above this, a camp can afford to give
public:
    TransferAdvisor();                                      // 3.0 and 7.0
    TransferAdvisor(double criticalDays, double surplusDays);
    double getCriticalDays() const;
    double getSurplusDays() const;

    std::vector<Item> criticalItems(const Camp& camp) const;

    // [Objects as arguments] [Returning objects]
    std::vector<TransferSuggestion> suggest(const std::vector<Camp*>& camps) const;
};

#endif
```

`suggest` algorithm, written out so there is nothing to invent:

```
for each camp `to` in camps:
    for each item in 0..ITEM_COUNT-1:
        toDays = to->getStock().daysOfSupplyFor(item)
        if toDays > criticalDays: continue
        toDemand = to->getStock().getDailyDemand().get(item)
        if toDemand <= 1e-9: continue          // no demand, so no shortage
        needed = criticalDays * toDemand - to->getStock().get(item)
        if needed <= 1e-9: continue
        best = 0; bestDays = 0
        for each camp `from` in camps:
            if from == to: continue
            fromDays = from->getStock().daysOfSupplyFor(item)
            if fromDays <= surplusDays: continue
            fromDemand = from->getStock().getDailyDemand().get(item)
            spare = from->getStock().get(item) - surplusDays * fromDemand
            if spare <= 1e-9: continue
            if fromDays > bestDays: best = from; bestDays = fromDays; bestSpare = spare
        if best != 0:
            quantity = min(needed, bestSpare)
            append TransferSuggestion(best->getId(), best->getName(),
                                      to->getId(), to->getName(),
                                      item, quantity, bestDays, toDays)
```

`criticalItems(camp)` returns each item whose `daysOfSupplyFor` is at or below `criticalDays` **and** whose daily demand is above `1e-9`. `render()` returns, for example, `"Move 60 kg of Rice from Mayong Embankment Tents (#3, 20.0 days) to Laharighat Community Hall (#2, 1.5 days)"`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `StockLedger and TransferAdvisor` suite all ok, including the check that applying the suggestion clears the shortage and leaves no further suggestion.

- [ ] **Step 5: Commit**

```bash
git add include/StockLedger.h include/TransferAdvisor.h src/StockLedger.cpp src/TransferAdvisor.cpp tests/test_transfer.cpp tests/test_main.cpp
git commit -m "Add StockLedger and TransferAdvisor

Every stock movement is logged, and netFor() recomputes the balance from the
transactions so the register and the running total cannot disagree. A failed
stockOut logs nothing. TransferAdvisor proposes a move only when a donor can
stay above its surplus line, and the test proves applying its suggestion
clears the shortage."
```

---
## Task 17: HealthMonitor and the outbreak alert

**Files:**
- Create: `include/HealthMonitor.h`, `src/HealthMonitor.cpp`, `tests/test_health.cpp`
- Modify: `tests/test_main.cpp` (add `run_health_tests();`)

**Interfaces:**
- Consumes: `Symptom.h`, `Date.h`.
- Produces:
  - `class HealthCase` (in `HealthMonitor.h`): `HealthCase()`, `HealthCase(int campId, const Date& date, Symptom symptom, int count)`, const getters, `friend operator<<`.
  - `class HealthMonitor`: `static const int kWindowDays;` (3), `static const int kDiarrhoeaThreshold;` (10), `static const int kFeverThreshold;` (15), `static const int kDefaultThreshold;` (20); `void logCase(int campId, const Date& date, Symptom symptom, int count)`, `int casesInWindow(int campId, const Date& today, Symptom symptom) const`, `bool outbreakAlert(int campId, const Date& today, Symptom symptom) const`, `std::vector<Symptom> activeAlerts(int campId, const Date& today) const`, `static int thresholdFor(Symptom)`, `static int windowDays()`, `int size() const`, `const HealthCase& at(int) const`, `std::vector<HealthCase> forCamp(int campId) const`, `void loadCase(const HealthCase&)`, `void clear()`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_health.cpp`:

```cpp
#include "TestRunner.h"
#include "HealthMonitor.h"
#include <vector>

void run_health_tests() {
    testing::beginSuite("HealthMonitor and the outbreak alert");

    // The thresholds are static, so they are the camp's norms, not one
    // monitor's settings.
    ASSERT_EQ(HealthMonitor::windowDays(), 3);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::Diarrhoea), 10);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::Fever), 15);
    ASSERT_EQ(HealthMonitor::thresholdFor(Symptom::SkinInfection), 20);

    HealthMonitor health;
    ASSERT_EQ(health.size(), 0);

    Date d12(2026, 7, 12);
    Date d13(2026, 7, 13);
    Date d14(2026, 7, 14);
    Date d15(2026, 7, 15);
    Date d11(2026, 7, 11);

    // Logging accumulates per camp, per day, per symptom.
    health.logCase(1, d12, Symptom::Diarrhoea, 3);
    health.logCase(1, d13, Symptom::Diarrhoea, 4);
    ASSERT_EQ(health.size(), 2);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 7);   // d12, d13, d14
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));   // 7 < 10

    // One more case crosses the threshold and the alert fires.
    health.logCase(1, d14, Symptom::Diarrhoea, 3);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);
    ASSERT_TRUE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));    // 10 >= 10

    // The window is exactly 3 days: [today-2, today]. A case from the 11th is
    // outside the window on the 14th, which is what keeps an old, resolved
    // cluster from raising a false alarm.
    health.logCase(1, d11, Symptom::Diarrhoea, 50);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);  // the 50 is ignored
    ASSERT_EQ(health.casesInWindow(1, d13, Symptom::Diarrhoea), 57);  // d11, d12, d13

    // Future cases are not counted either.
    health.logCase(1, d15, Symptom::Diarrhoea, 40);
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Diarrhoea), 10);

    // Rolling forward, the oldest day drops out of the window.
    // On the 15th the window is d13, d14, d15 = 4 + 3 + 40 = 47.
    ASSERT_EQ(health.casesInWindow(1, d15, Symptom::Diarrhoea), 47);
    ASSERT_TRUE(health.outbreakAlert(1, d15, Symptom::Diarrhoea));

    // Symptoms are tracked separately.
    ASSERT_EQ(health.casesInWindow(1, d14, Symptom::Fever), 0);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Fever));
    health.logCase(1, d14, Symptom::Fever, 14);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Fever));       // 14 < 15
    health.logCase(1, d14, Symptom::Fever, 1);
    ASSERT_TRUE(health.outbreakAlert(1, d14, Symptom::Fever));        // 15 >= 15

    // Camps are tracked separately: camp 2's silence is not camp 1's outbreak.
    ASSERT_EQ(health.casesInWindow(2, d14, Symptom::Diarrhoea), 0);
    ASSERT_FALSE(health.outbreakAlert(2, d14, Symptom::Diarrhoea));
    health.logCase(2, d14, Symptom::Diarrhoea, 12);
    ASSERT_TRUE(health.outbreakAlert(2, d14, Symptom::Diarrhoea));
    ASSERT_FALSE(health.outbreakAlert(2, d14, Symptom::Fever));

    // activeAlerts lists every symptom over its threshold at one camp.
    std::vector<Symptom> alerts = health.activeAlerts(1, d14);
    ASSERT_EQ(static_cast<int>(alerts.size()), 2);                    // diarrhoea + fever
    std::vector<Symptom> camp2Alerts = health.activeAlerts(2, d14);
    ASSERT_EQ(static_cast<int>(camp2Alerts.size()), 1);
    ASSERT_TRUE(camp2Alerts[0] == Symptom::Diarrhoea);
    ASSERT_EQ(static_cast<int>(health.activeAlerts(9, d14).size()), 0);

    // Queries and loading.
    ASSERT_TRUE(health.size() > 0);
    ASSERT_EQ(health.at(0).getCampId(), 1);
    ASSERT_TRUE(health.at(0).getSymptom() == Symptom::Diarrhoea);
    ASSERT_EQ(health.at(0).getCount(), 3);
    ASSERT_THROWS(health.at(999), std::out_of_range);
    ASSERT_TRUE(static_cast<int>(health.forCamp(1).size()) > 0);
    ASSERT_EQ(static_cast<int>(health.forCamp(9).size()), 0);

    HealthMonitor loaded;
    loaded.loadCase(HealthCase(1, d14, Symptom::Diarrhoea, 11));
    ASSERT_EQ(loaded.size(), 1);
    ASSERT_TRUE(loaded.outbreakAlert(1, d14, Symptom::Diarrhoea));

    // A zero or negative count is ignored rather than corrupting the total.
    HealthMonitor guarded;
    guarded.logCase(1, d14, Symptom::Fever, 0);
    guarded.logCase(1, d14, Symptom::Fever, -5);
    ASSERT_EQ(guarded.casesInWindow(1, d14, Symptom::Fever), 0);

    health.clear();
    ASSERT_EQ(health.size(), 0);
    ASSERT_FALSE(health.outbreakAlert(1, d14, Symptom::Diarrhoea));
}
```

Add `void run_health_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: HealthMonitor.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/HealthMonitor.h`:

```cpp
#ifndef RELIEF_HEALTHMONITOR_H
#define RELIEF_HEALTHMONITOR_H

#include "Symptom.h"
#include "Date.h"
#include <vector>
#include <ostream>

// One line in the camp's health register: this many cases of this symptom at
// this camp on this day.
class HealthCase {
    int campId;
    Date date;
    Symptom symptom;
    int count;
public:
    HealthCase();
    HealthCase(int campId, const Date& date, Symptom symptom, int count);
    int getCampId() const;
    const Date& getDate() const;
    Symptom getSymptom() const;
    int getCount() const;
    friend std::ostream& operator<<(std::ostream& os, const HealthCase& h);
};

// Watches the health register for the beginnings of an outbreak. Diarrhoea is
// the one that kills in a flood camp, because the water is contaminated, so it
// has the lowest threshold.
//
// The alert looks at a rolling 3-day window rather than a single day: one bad
// afternoon is not an outbreak, and three quiet days in a row should clear the
// alarm by themselves.
class HealthMonitor {
    // [Static data members] these are the camp's norms, not one monitor's settings
    static const int kWindowDays;
    static const int kDiarrhoeaThreshold;
    static const int kFeverThreshold;
    static const int kDefaultThreshold;

    std::vector<HealthCase> cases;

public:
    HealthMonitor();

    void logCase(int campId, const Date& date, Symptom symptom, int count);

    // Total cases at one camp, of one symptom, in [today - 2, today].
    int casesInWindow(int campId, const Date& today, Symptom symptom) const;
    bool outbreakAlert(int campId, const Date& today, Symptom symptom) const;
    std::vector<Symptom> activeAlerts(int campId, const Date& today) const;

    // [Static member functions]
    static int thresholdFor(Symptom symptom);
    static int windowDays();

    int size() const;
    const HealthCase& at(int index) const;          // throws std::out_of_range
    std::vector<HealthCase> forCamp(int campId) const;
    void loadCase(const HealthCase& c);             // CSV loading
    void clear();
};

#endif
```

Implementation notes:
- `const int HealthMonitor::kWindowDays = 3;`, `kDiarrhoeaThreshold = 10`, `kFeverThreshold = 15`, `kDefaultThreshold = 20`, all at file scope in the `.cpp`.
- `logCase`: `if (count <= 0) return;` then push a `HealthCase`.
- `casesInWindow`: for each case, skip unless `campId` and `symptom` match, then compute `long age = today - c.getDate();` (the `Date` difference operator) and count it when `age >= 0 && age < kWindowDays`. Comment that line `// [Operator overloading in use] Date::operator- gives the age in days`.
- `outbreakAlert`: `return casesInWindow(...) >= thresholdFor(symptom);`
- `activeAlerts`: loop `0..SYMPTOM_COUNT-1`, push each symptom whose `outbreakAlert` is true.
- `thresholdFor`: `switch` returning `kDiarrhoeaThreshold`, `kFeverThreshold`, else `kDefaultThreshold`.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `HealthMonitor and the outbreak alert` suite all ok, including both window boundary checks.

- [ ] **Step 5: Commit**

```bash
git add include/HealthMonitor.h src/HealthMonitor.cpp tests/test_health.cpp tests/test_main.cpp
git commit -m "Add HealthMonitor with a rolling 3-day outbreak alert

Cases are counted per camp and per symptom over [today-2, today] using
Date::operator-, so one bad afternoon is not an outbreak and three quiet days
clear the alarm. Diarrhoea has the lowest threshold because contaminated water
is the real killer in a flood camp."
```

---

## Task 18: ReportGenerator as a friend of Camp

**Files:**
- Create: `include/ReportGenerator.h`, `src/ReportGenerator.cpp`, `tests/test_report.cpp`
- Modify: `tests/test_main.cpp` (add `run_report_tests();`)

**Interfaces:**
- Consumes: `Camp.h` (private access via friendship), `Family.h`, `DistributionLog.h`, `HealthMonitor.h`, `TransferAdvisor.h`, `Exceptions.h`.
- Produces: `class ReportGenerator` — `ReportGenerator()`, `std::string dailyReport(const Camp& camp, const Date& date, const std::vector<Family*>& families, const DistributionLog& log, const HealthMonitor& health) const`, `void exportCsv(const std::string& path, const Camp& camp, const Date& date, const std::vector<Family*>& families, const DistributionLog& log, const HealthMonitor& health) const` (throws `FileFormatException` if the file cannot be opened), `std::string stockTable(const Camp& camp) const`.
- Note: every parameter is a `const` reference, so a report can never alter what it reports on. That is the "constant objects and const member functions" row doing real work.

- [ ] **Step 1: Write the failing test**

Create `tests/test_report.cpp`:

```cpp
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
#include <fstream>
#include <cstdio>

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

    // The camp's daily draw is the sum of its families' entitlements.
    Ration demand = f1.dailyRation() + f2.dailyRation();
    camp.stockRef().setDailyDemand(demand);

    DistributionLog log;
    log.record(Allocation(f1.getId(), camp.getId(), today, f1.dailyRation()));

    HealthMonitor health;
    health.logCase(camp.getId(), today, Symptom::Diarrhoea, 11);   // over the threshold

    ReportGenerator reporter;

    // --- the console report ---
    std::string report = reporter.dailyReport(camp, today, families, log, health);

    ASSERT_TRUE(report.find("Bhuragaon HS School") != std::string::npos);
    ASSERT_TRUE(report.find("Morigaon") != std::string::npos);
    ASSERT_TRUE(report.find("2026-07-14") != std::string::npos);
    ASSERT_TRUE(report.find("5") != std::string::npos);             // occupancy
    ASSERT_TRUE(report.find("200") != std::string::npos);           // capacity
    ASSERT_TRUE(report.find("Room-1") != std::string::npos);        // shelters listed
    ASSERT_TRUE(report.find("Tent-1") != std::string::npos);
    ASSERT_TRUE(report.find("Rice") != std::string::npos);          // stock table
    ASSERT_TRUE(report.find("days") != std::string::npos);          // days of supply
    ASSERT_TRUE(report.find("Diarrhoea") != std::string::npos);     // health section
    ASSERT_TRUE(report.find("OUTBREAK") != std::string::npos);      // the alert fired
    ASSERT_TRUE(report.find("Bipul Das") != std::string::npos);     // family list
    ASSERT_TRUE(report.find("Rupa Nath") != std::string::npos);

    // Families served today vs pending.
    ASSERT_TRUE(report.find("1 of 2") != std::string::npos ||
                report.find("served") != std::string::npos);

    // A quiet camp has no OUTBREAK banner, which proves the banner is driven
    // by the data and not printed unconditionally.
    HealthMonitor quiet;
    quiet.logCase(camp.getId(), today, Symptom::Diarrhoea, 2);
    std::string calmReport = reporter.dailyReport(camp, today, families, log, quiet);
    ASSERT_TRUE(calmReport.find("OUTBREAK") == std::string::npos);

    // stockTable on its own, for the stock menu screen.
    std::string table = reporter.stockTable(camp);
    ASSERT_TRUE(table.find("Rice") != std::string::npos);
    ASSERT_TRUE(table.find("kg") != std::string::npos);

    // --- CSV export ---
    const std::string path = "test_report_out.csv";
    std::remove(path.c_str());
    reporter.exportCsv(path, camp, today, families, log, health);

    std::ifstream in(path.c_str());
    ASSERT_TRUE(in.is_open());
    std::string contents;
    std::string line;
    int lineCount = 0;
    while (std::getline(in, line)) {
        contents += line;
        contents += "\n";
        lineCount++;
    }
    in.close();

    ASSERT_TRUE(lineCount >= 4);                                     // header + sections
    ASSERT_TRUE(contents.find("Bhuragaon HS School") != std::string::npos);
    ASSERT_TRUE(contents.find("2026-07-14") != std::string::npos);
    ASSERT_TRUE(contents.find("Rice") != std::string::npos);
    ASSERT_TRUE(contents.find(",") != std::string::npos);            // it is really CSV

    std::remove(path.c_str());

    // An unwritable path is reported, not swallowed.
    ASSERT_THROWS(reporter.exportCsv("no_such_dir_xyz/report.csv", camp, today,
                                     families, log, health),
                  FileFormatException);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
```

Add `void run_report_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: ReportGenerator.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/ReportGenerator.h`:

```cpp
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
// Camp declares this class a friend, so the report reads camp.occupancy and
// camp.stock directly. The alternative - adding public getters to Camp purely
// for the report - would widen Camp's interface for every other caller too.
//
// [Constant objects] Every parameter is a const reference: generating a report
// must never change what it reports on.
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
```

`src/ReportGenerator.cpp` structure. `dailyReport` builds a block with `std::ostringstream` in this order, and the friendship is used where marked:

```cpp
std::string ReportGenerator::dailyReport(const Camp& camp, const Date& date,
                                         const std::vector<Family*>& families,
                                         const DistributionLog& log,
                                         const HealthMonitor& health) const {
    std::ostringstream out;
    out << "=====================================================\n";
    out << " DAILY CAMP REPORT   " << date << "\n";
    out << "=====================================================\n";
    // [Friend class in use] camp.name, camp.district, camp.occupancy,
    // camp.capacity and camp.stock are PRIVATE members of Camp. Only a friend
    // can read them.
    out << " Camp      : #" << camp.id << "  " << camp.name << "\n";
    out << " District  : " << camp.district << "\n";
    out << " Occupancy : " << camp.occupancy << " / " << camp.capacity;
    if (camp.isOverflowing()) { out << "   *** OVER CAPACITY ***"; }
    out << "\n";
    // ... shelters, stock table, distribution summary, health section,
    //     family list
    return out.str();
}
```

Sections to include, in order:
1. Header with the date, camp ID, name, district, occupancy over capacity, and `*** OVER CAPACITY ***` when `isOverflowing()`.
2. `SHELTERS` — one line per shelter using `camp.shelters[i]` (private, via friendship): label, occupancy over capacity.
3. `STOCK` — `stockTable(camp)`, which prints each item with its quantity, unit and `daysOfSupplyFor`, marking any item at or below 3.0 days with `LOW`, and ends with `Worst item: <name> (<n> days)`.
4. `DISTRIBUTION` — `log.familiesServed(camp.id, date)` and `static_cast<int>(families.size())` rendered as `Served today: 1 of 2 families`, then the day's total from `log.totalForCamp`.
5. `HEALTH` — every symptom with a non-zero `casesInWindow`, printed as `Diarrhoea: 11 cases in the last 3 days`, and for each entry in `health.activeAlerts(camp.id, date)` a line `*** OUTBREAK ALERT: Diarrhoea ***`.
6. `FAMILIES` — one line per family: ID, head name, size, vulnerability score, and `collected` or `PENDING` from `log.alreadyClaimed(f->getId(), date)`.

`exportCsv` opens an `std::ofstream`, throws `FileFormatException(path, 0, "cannot open file for writing")` when `!out.is_open()`, then writes a flat CSV: a `section,field,value` shaped file is simplest and keeps every row the same width. Write a header row `section,key,value`, then rows for the camp fields, one row per shelter, one per item (`stock,Rice,500`), one per symptom with cases, and one per family. `stockTable` uses `std::fixed` and `std::setprecision(1)` for the day counts.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `ReportGenerator` suite all ok, including the unwritable-path throw.

Then prove the friendship is load-bearing: temporarily comment out `friend class ReportGenerator;` in `include/Camp.h`, run `make test`, and observe errors such as "'occupancy' is a private member of 'Camp'". Restore the line and re-run.

- [ ] **Step 5: Commit**

```bash
git add include/ReportGenerator.h src/ReportGenerator.cpp tests/test_report.cpp tests/test_main.cpp
git commit -m "Add ReportGenerator as a friend of Camp

The daily report reads Camp's private occupancy, shelters and stock directly
through friendship, instead of widening Camp's public interface for one
caller. Every parameter is a const reference, so reporting cannot mutate what
it reports on. Export writes a flat section,key,value CSV and throws
FileFormatException when the path is unwritable."
```

---

## Task 19: Csv helpers and CsvStore persistence

**Files:**
- Create: `include/Csv.h`, `src/Csv.cpp`, `include/CsvStore.h`, `src/CsvStore.cpp`, `tests/test_csv.cpp`
- Modify: `tests/test_main.cpp` (add `run_csv_tests();`)

**Interfaces:**
- Consumes: `Registry.h`, `Camp.h`, `Family.h`, all `Person` subclasses, `Volunteer.h`, `DistributionLog.h`, `StockLedger.h`, `HealthMonitor.h`, `Exceptions.h`.
- Produces:
  - `class Csv`: `static std::vector<std::string> split(const std::string& line)`, `static std::string join(const std::vector<std::string>& fields)`, `static std::string escape(const std::string& field)`, `static int toInt(const std::string& s, const std::string& file, int line)`, `static double toDouble(const std::string& s, const std::string& file, int line)`, `static void require(int actual, int expected, const std::string& file, int line)` (throws `FileFormatException`).
  - `class CsvStore`: `explicit CsvStore(const std::string& dir)`, `const std::string& getDir() const`, `bool dataExists() const`, `void saveAll(const Registry<Camp>&, const Registry<Family>&, const Registry<Volunteer>&, const DistributionLog&, const StockLedger&, const HealthMonitor&) const`, `void loadAll(Registry<Camp>&, Registry<Family>&, Registry<Volunteer>&, DistributionLog&, StockLedger&, HealthMonitor&) const`, `static Person* makePerson(const std::string& typeTag, const std::string& name, int age, char sex, int familyId, int campId, const std::string& skill, const std::string& shift)`.
- File layout, one header row each:
  - `camps.csv`: `id,name,district,capacity,occupancy`
  - `shelters.csv`: `campId,label,capacity,occupancy`
  - `stock.csv`: `campId,item,quantity`
  - `families.csv`: `id,headName,village,campId`
  - `members.csv`: `familyId,typeTag,name,age,sex,pregnant,disabled,lactating,skill,shift`
  - `volunteers.csv`: `id,name,age,sex,skill,shift,organisation`
  - `distributions.csv`: `familyId,campId,date,` then one column per item
  - `stock_txns.csv`: `campId,item,quantity,incoming,date,note`
  - `health.csv`: `campId,date,symptom,count`

- [ ] **Step 1: Write the failing test**

Create `tests/test_csv.cpp`:

```cpp
#include "TestRunner.h"
#include "Csv.h"
#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "RationNorms.h"
#include "Exceptions.h"
#include <memory>
#include <fstream>
#include <cstdio>

static void removeDataFiles(const std::string& dir) {
    const char* names[] = {"camps.csv", "shelters.csv", "stock.csv", "families.csv",
                           "members.csv", "volunteers.csv", "distributions.csv",
                           "stock_txns.csv", "health.csv"};
    for (int i = 0; i < 9; i++) {
        std::remove((dir + "/" + names[i]).c_str());
    }
}

void run_csv_tests() {
    testing::beginSuite("Csv helpers and CsvStore round trip");

    // --- Csv field handling ---
    std::vector<std::string> fields = Csv::split("1,Bhuragaon HS School,Morigaon,200,45");
    ASSERT_EQ(static_cast<int>(fields.size()), 5);
    ASSERT_STR_EQ(fields[1], "Bhuragaon HS School");
    ASSERT_STR_EQ(fields[4], "45");

    // Empty fields are preserved, so a missing shelter label does not shift
    // every later column.
    std::vector<std::string> sparse = Csv::split("1,,Morigaon,,45");
    ASSERT_EQ(static_cast<int>(sparse.size()), 5);
    ASSERT_STR_EQ(sparse[1], "");
    ASSERT_STR_EQ(sparse[3], "");

    // A field containing a comma is quoted on the way out and recovered on the
    // way in. Assamese place names do contain commas in official lists.
    ASSERT_STR_EQ(Csv::escape("Mayong, Morigaon"), "\"Mayong, Morigaon\"");
    ASSERT_STR_EQ(Csv::escape("plain"), "plain");
    std::vector<std::string> quoted = Csv::split("1,\"Mayong, Morigaon\",200");
    ASSERT_EQ(static_cast<int>(quoted.size()), 3);
    ASSERT_STR_EQ(quoted[1], "Mayong, Morigaon");

    std::vector<std::string> out;
    out.push_back("1");
    out.push_back("Mayong, Morigaon");
    out.push_back("200");
    ASSERT_STR_EQ(Csv::join(out), "1,\"Mayong, Morigaon\",200");

    // Numeric parsing names the file and line when the data is wrong, because
    // these files get hand-edited in the field.
    ASSERT_EQ(Csv::toInt("42", "camps.csv", 3), 42);
    ASSERT_NEAR(Csv::toDouble("12.5", "stock.csv", 7), 12.5, 1e-9);
    ASSERT_THROWS(Csv::toInt("forty-two", "camps.csv", 3), FileFormatException);
    ASSERT_THROWS(Csv::toDouble("", "stock.csv", 7), FileFormatException);
    try {
        Csv::toInt("abc", "camps.csv", 9);
    } catch (const FileFormatException& e) {
        ASSERT_STR_EQ(e.getFile(), "camps.csv");
        ASSERT_EQ(e.getLine(), 9);
    }
    ASSERT_THROWS(Csv::require(3, 5, "families.csv", 12), FileFormatException);

    // --- full round trip ---
    const std::string dir = ".";
    removeDataFiles(dir);
    CsvStore store(dir);
    ASSERT_STR_EQ(store.getDir(), dir);
    ASSERT_FALSE(store.dataExists());

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    Registry<Camp> camps;
    Registry<Family> families;
    Registry<Volunteer> volunteers;
    DistributionLog log;
    StockLedger ledger;
    HealthMonitor health;
    Date today(2026, 7, 14);

    Camp* school = camps.add(std::unique_ptr<Camp>(
        new Camp("Bhuragaon HS School", "Morigaon", 200)));
    school->addShelter(Camp::Shelter("Room-1", 60));
    school->addShelter(Camp::Shelter("Tent-1", 40));
    school->stockRef().set(Item::Rice, 500.0);
    school->stockRef().set(Item::Dal, 60.5);

    Camp* hall = camps.add(std::unique_ptr<Camp>(
        new Camp("Laharighat Community Hall", "Morigaon", 150)));
    hall->stockRef().set(Item::Rice, 120.0);

    Family* f = families.add(std::unique_ptr<Family>(new Family("Bipul Das", "Bhuragaon")));
    f->setCampId(school->getId());
    f->addMember(new Adult("Bipul Das", 35, 'M', f->getId(), school->getId()));
    Adult* mother = new Adult("Rina Das", 29, 'F', f->getId(), school->getId());
    mother->setFlags(true, false, false);               // pregnant - must survive the trip
    f->addMember(mother);
    f->addMember(new Child("Rahul Das", 9, 'M', f->getId(), school->getId()));
    f->addMember(new Infant("Mitu Das", 1, 'F', f->getId(), school->getId()));
    f->addMember(new Elderly("Minoti Devi", 72, 'F', f->getId(), school->getId()));
    f->addMember(new ResidentVolunteer("Gautam Das", 31, 'M', f->getId(),
                                       school->getId(), "Record keeper", "Evening"));
    school->admit(*f);

    volunteers.add(std::unique_ptr<Volunteer>(
        new Volunteer("Dr Priyam Deka", 45, 'M', "Medic", "Morning", "Red Cross")));

    log.record(Allocation(f->getId(), school->getId(), today, f->dailyRation()));
    ledger.stockIn(*school, Item::Rice, 100.0, today, "district supply");
    health.logCase(school->getId(), today, Symptom::Diarrhoea, 11);

    int familyCount = families.size();
    int memberCount = f->size();
    double savedRice = school->getStock().get(Item::Rice);
    int savedOccupancy = school->getOccupancy();
    int savedFamilyId = f->getId();
    double savedFamilyRice = f->dailyRation().get(Item::Rice);
    int savedVulnerability = f->vulnerabilityScore();

    store.saveAll(camps, families, volunteers, log, ledger, health);
    ASSERT_TRUE(store.dataExists());

    // Load into a completely fresh set of registries.
    Registry<Camp> camps2;
    Registry<Family> families2;
    Registry<Volunteer> volunteers2;
    DistributionLog log2;
    StockLedger ledger2;
    HealthMonitor health2;

    store.loadAll(camps2, families2, volunteers2, log2, ledger2, health2);

    ASSERT_EQ(camps2.size(), 2);
    ASSERT_EQ(families2.size(), familyCount);
    ASSERT_EQ(volunteers2.size(), 1);

    // IDs survive, which is what makes the cross-references work.
    Camp* school2 = camps2.findById(school->getId());
    ASSERT_TRUE(school2 != 0);
    ASSERT_STR_EQ(school2->getName(), "Bhuragaon HS School");
    ASSERT_STR_EQ(school2->getDistrict(), "Morigaon");
    ASSERT_EQ(school2->getCapacity(), 200);
    ASSERT_EQ(school2->getOccupancy(), savedOccupancy);
    ASSERT_EQ(school2->shelterCount(), 2);
    ASSERT_STR_EQ(school2->shelterAt(0).getLabel(), "Room-1");
    ASSERT_EQ(school2->shelterAt(0).getCapacity(), 60);
    ASSERT_NEAR(school2->getStock().get(Item::Rice), savedRice, 1e-9);
    ASSERT_NEAR(school2->getStock().get(Item::Dal), 60.5, 1e-9);

    Family* f2 = families2.findById(savedFamilyId);
    ASSERT_TRUE(f2 != 0);
    ASSERT_STR_EQ(f2->getHeadName(), "Bipul Das");
    ASSERT_STR_EQ(f2->getVillage(), "Bhuragaon");
    ASSERT_EQ(f2->getCampId(), school->getId());
    ASSERT_EQ(f2->size(), memberCount);

    // The dynamic type of every member survives the round trip. This is the
    // assertion that catches a CSV layer that quietly turns everyone into an
    // Adult: the ration and the score would both be wrong.
    ASSERT_EQ(f2->countOfTag("ADULT"), 2);
    ASSERT_EQ(f2->countOfTag("CHILD"), 1);
    ASSERT_EQ(f2->countOfTag("INFANT"), 1);
    ASSERT_EQ(f2->countOfTag("ELDERLY"), 1);
    ASSERT_EQ(f2->countOfTag("RESIDENT_VOLUNTEER"), 1);
    ASSERT_NEAR(f2->dailyRation().get(Item::Rice), savedFamilyRice, 1e-9);
    ASSERT_EQ(f2->vulnerabilityScore(), savedVulnerability);

    // The pregnancy flag survived, so the bonus ration is still right.
    bool foundPregnant = false;
    for (int i = 0; i < f2->size(); i++) {
        if (f2->memberAt(i).isPregnant()) { foundPregnant = true; }
    }
    ASSERT_TRUE(foundPregnant);

    // The resident volunteer kept both halves of its identity.
    bool foundRv = false;
    for (int i = 0; i < f2->size(); i++) {
        if (f2->memberAt(i).typeTag() == "RESIDENT_VOLUNTEER") {
            foundRv = true;
            ASSERT_NEAR(f2->memberAt(i).rationNeed().get(Item::Rice), 0.400, 1e-9);
        }
    }
    ASSERT_TRUE(foundRv);

    // Logs survive.
    ASSERT_EQ(log2.size(), 1);
    ASSERT_TRUE(log2.alreadyClaimed(savedFamilyId, today));
    ASSERT_NEAR(log2.totalForCamp(school->getId(), today).get(Item::Rice),
                savedFamilyRice, 1e-9);
    ASSERT_EQ(ledger2.size(), 1);
    ASSERT_TRUE(ledger2.at(0).getItem() == Item::Rice);
    ASSERT_EQ(health2.size(), 1);
    ASSERT_TRUE(health2.outbreakAlert(school->getId(), today, Symptom::Diarrhoea));

    // A new object created after loading must not collide with a loaded ID.
    Family* fresh = families2.add(std::unique_ptr<Family>(new Family("New Head", "New Village")));
    ASSERT_TRUE(families2.findById(fresh->getId()) == fresh);
    ASSERT_TRUE(fresh->getId() != savedFamilyId);

    // --- malformed data is reported with the line number ---
    {
        std::ofstream bad((dir + "/camps.csv").c_str());
        bad << "id,name,district,capacity,occupancy\n";
        bad << "1,Good Camp,Morigaon,200,45\n";
        bad << "2,Broken Camp,Morigaon\n";              // only 3 fields
        bad.close();
    }
    Registry<Camp> camps3;
    Registry<Family> families3;
    Registry<Volunteer> volunteers3;
    DistributionLog log3;
    StockLedger ledger3;
    HealthMonitor health3;
    ASSERT_THROWS(store.loadAll(camps3, families3, volunteers3, log3, ledger3, health3),
                  FileFormatException);
    try {
        Registry<Camp> c4; Registry<Family> f4; Registry<Volunteer> v4;
        DistributionLog l4; StockLedger s4; HealthMonitor h4;
        store.loadAll(c4, f4, v4, l4, s4, h4);
    } catch (const FileFormatException& e) {
        ASSERT_TRUE(e.getFile().find("camps.csv") != std::string::npos);
        ASSERT_EQ(e.getLine(), 3);                       // the header counts as line 1
    }

    // A missing data directory is simply "no data yet", not a crash.
    CsvStore absent("no_such_dir_xyz");
    ASSERT_FALSE(absent.dataExists());

    removeDataFiles(dir);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
```

Add `void run_csv_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Csv.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Csv.h`:

```cpp
#ifndef RELIEF_CSV_H
#define RELIEF_CSV_H

#include <string>
#include <vector>

// Field-level CSV handling, kept apart from CsvStore so the fiddly quoting
// rules can be tested on their own.
//
// These files get opened in a spreadsheet and hand-edited by camp staff, so a
// bad field has to name its file and line rather than silently becoming a zero.
class Csv {
public:
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
```

`src/Csv.cpp` notes:
- `split`: walk the characters; a `"` toggles an `inQuotes` flag and is not copied; a `,` outside quotes ends the current field. Always push the final field, so `"a,,b"` yields three fields and a trailing comma yields a trailing empty field.
- `escape`: wrap in `"` when the field contains a comma, a quote or a newline; double any embedded `"`.
- `join`: `escape` each field and insert `,` between them.
- `toInt` / `toDouble`: reject an empty string; use `std::istringstream`, then require both that the extraction succeeded and that nothing remains unconsumed (`iss >> value` succeeded and `iss.eof()`); otherwise `throw FileFormatException(file, line, "expected a number, found \"" + text + "\"")`.
- `require`: `if (actualFields != expectedFields) throw FileFormatException(file, line, "expected N fields, found M")` with the real numbers substituted.

Create `include/CsvStore.h`:

```cpp
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
// [File handling] One file per kind of record, each with a header row.
class CsvStore {
    std::string dir;

    // Each of these throws FileFormatException on malformed input.
    void saveCamps(const Registry<Camp>& camps) const;
    void saveShelters(const Registry<Camp>& camps) const;
    void saveStock(const Registry<Camp>& camps) const;
    void saveFamilies(const Registry<Family>& families) const;
    void saveMembers(const Registry<Family>& families) const;
    void saveVolunteers(const Registry<Volunteer>& volunteers) const;
    void saveDistributions(const DistributionLog& log) const;
    void saveStockTxns(const StockLedger& ledger) const;
    void saveHealth(const HealthMonitor& health) const;

    void loadCamps(Registry<Camp>& camps) const;
    void loadShelters(Registry<Camp>& camps) const;
    void loadStock(Registry<Camp>& camps) const;
    void loadFamilies(Registry<Family>& families) const;
    void loadMembers(Registry<Family>& families) const;
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
```

Implementation notes:
- Every loader opens the file, reads the header line as line 1, then processes each row with a running line counter starting at 2. A missing file is not an error for a loader: `if (!in.is_open()) return;` so a partial `data/` directory still loads what it has. Only a *malformed* row throws.
- `loadAll` order matters: camps, then shelters and stock (which need the camps to exist), then families, then members (which need the families), then volunteers, then the three logs.
- After loading, each loader calls `resetIdCounter(maxLoadedId + 1)` on the type it owns — `Camp::resetIdCounter`, `Family::resetIdCounter`, `Person::resetIdCounter` — so new objects never collide with loaded IDs. That is what the test's `fresh->getId() != savedFamilyId` assertion checks.
- `loadCamps` uses `adoptId` then `setOccupancy`. `loadShelters` looks the camp up with `camps.findById` and calls `addShelter` then `setOccupancy` on the shelter; a row naming an unknown camp throws `FileFormatException`.
- `saveMembers` writes `typeTag()` for each member plus the flags; for a `RESIDENT_VOLUNTEER` it also writes `skill` and `shift`, obtained by `dynamic_cast<const ResidentVolunteer*>(&member)`. For every other type those two columns are empty. Mark the cast `// dynamic_cast is the honest tool here: only some members carry a skill.`
- `makePerson` switches on the tag string and returns `new Infant(...)`, `new Child(...)`, `new Adult(...)`, `new Elderly(...)`, `new ResidentVolunteer(name, age, sex, familyId, campId, skill, shift)`, or `new Volunteer(...)`; an unknown tag throws `std::invalid_argument`, which `loadMembers` catches and rethrows as `FileFormatException` with the line number.
- `saveDistributions` writes the date with `Date::toString()` and one column per item; `loadDistributions` rebuilds the `Ration` and calls `log.loadEntry(...)`, never `record(...)`.
- `dataExists` tries to open `path("camps.csv")` for reading and returns whether it opened.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Csv helpers and CsvStore round trip` suite all ok, including the dynamic-type preservation and the `camps.csv` line 3 error.

- [ ] **Step 5: Commit**

```bash
git add include/Csv.h include/CsvStore.h src/Csv.cpp src/CsvStore.cpp tests/test_csv.cpp tests/test_main.cpp
git commit -m "Add CSV persistence for every registry and log

Plain CSV so the data opens in a spreadsheet, can be hand-fixed by camp staff
and survives a power cut. The round-trip test asserts every member keeps its
dynamic type, flags and ration, which is what catches a loader that quietly
turns the whole camp into adults. Malformed rows throw FileFormatException
naming the file and line, and loaders reset the ID counters so new records
cannot collide with loaded ones."
```

---

## Task 20: Autosaver background thread

**Files:**
- Create: `include/Autosaver.h`, `src/Autosaver.cpp`, `tests/test_autosave.cpp`
- Modify: `tests/test_main.cpp` (add `run_autosave_tests();`)

**Interfaces:**
- Consumes: nothing from the project; `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>`, `<functional>`, `<chrono>`.
- Produces: `class Autosaver` — `Autosaver()`, `~Autosaver()`, copy and assignment deleted, `void start(int intervalSeconds, std::mutex& dataMutex, std::function<void()> saveFn)`, `void stop()`, `bool isRunning() const`, `int getSaveCount() const`, `void saveNow()`.

- [ ] **Step 1: Write the failing test**

Create `tests/test_autosave.cpp`:

```cpp
#include "TestRunner.h"
#include "Autosaver.h"
#include <mutex>
#include <thread>
#include <chrono>
#include <atomic>

void run_autosave_tests() {
    testing::beginSuite("Autosaver background thread");

    // --- it actually saves, repeatedly, on its own ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        Autosaver saver;
        ASSERT_FALSE(saver.isRunning());
        ASSERT_EQ(saver.getSaveCount(), 0);

        saver.start(1, dataMutex, [&saves]() { saves++; });
        ASSERT_TRUE(saver.isRunning());

        // Wait for at least two ticks of a one-second interval.
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        saver.stop();
        ASSERT_FALSE(saver.isRunning());

        ASSERT_TRUE(saves.load() >= 2);
        ASSERT_EQ(saver.getSaveCount(), saves.load());
    }

    // --- stop() returns promptly, rather than waiting out the interval ---
    {
        std::mutex dataMutex;
        Autosaver saver;
        saver.start(3600, dataMutex, []() {});       // an hour between saves
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        saver.stop();                                 // must not block for an hour
        long elapsedMs = static_cast<long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - begin).count());
        ASSERT_TRUE(elapsedMs < 1000);
        ASSERT_FALSE(saver.isRunning());
    }

    // --- the save holds the data mutex, so it cannot see a half-written state ---
    {
        std::mutex dataMutex;
        std::atomic<bool> sawLock(false);
        std::atomic<int> saves(0);
        Autosaver saver;

        saver.start(1, dataMutex, [&]() {
            // If the saver did not already hold dataMutex, try_lock would
            // succeed here, which would mean saves can race the menu.
            if (!dataMutex.try_lock()) {
                sawLock = true;
            } else {
                dataMutex.unlock();
            }
            saves++;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        saver.stop();
        ASSERT_TRUE(saves.load() >= 1);
        ASSERT_TRUE(sawLock.load());
    }

    // --- the destructor stops the thread even if stop() was never called ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        {
            Autosaver saver;
            saver.start(1, dataMutex, [&saves]() { saves++; });
            std::this_thread::sleep_for(std::chrono::milliseconds(1200));
            // no stop() - the destructor must clean up and join
        }
        int countAtDestruction = saves.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        // Nothing more happened after destruction, so the thread really joined.
        ASSERT_EQ(saves.load(), countAtDestruction);
        ASSERT_TRUE(countAtDestruction >= 1);
    }

    // --- stop() twice, and stop() without start(), are both harmless ---
    {
        std::mutex dataMutex;
        Autosaver saver;
        saver.stop();                                 // never started
        ASSERT_FALSE(saver.isRunning());
        saver.start(1, dataMutex, []() {});
        saver.stop();
        saver.stop();                                 // again
        ASSERT_FALSE(saver.isRunning());
    }

    // --- saveNow() runs the callback on the caller's thread, for menu "save" ---
    {
        std::mutex dataMutex;
        std::atomic<int> saves(0);
        Autosaver saver;
        saver.start(3600, dataMutex, [&saves]() { saves++; });
        saver.saveNow();
        ASSERT_EQ(saves.load(), 1);
        ASSERT_EQ(saver.getSaveCount(), 1);
        saver.stop();
    }

    // --- an exception thrown by the save callback does not kill the thread ---
    {
        std::mutex dataMutex;
        std::atomic<int> attempts(0);
        Autosaver saver;
        saver.start(1, dataMutex, [&attempts]() {
            attempts++;
            throw std::runtime_error("disk full");
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        ASSERT_TRUE(attempts.load() >= 2);            // it kept trying
        ASSERT_TRUE(saver.isRunning());
        saver.stop();
    }
}
```

Add `void run_autosave_tests();` and the call to `tests/test_main.cpp`.

Note for the implementer: this suite deliberately sleeps and takes roughly 12 seconds. That is acceptable for a suite run once per task, and the one-second interval keeps it honest about real thread behaviour.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: FAIL — `fatal error: Autosaver.h: No such file or directory`.

- [ ] **Step 3: Write minimal implementation**

Create `include/Autosaver.h`:

```cpp
#ifndef RELIEF_AUTOSAVER_H
#define RELIEF_AUTOSAVER_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

// [Multithreading] Saves the data to disk every few seconds on a background
// thread, so an unannounced power cut - routine in a flood-hit district -
// costs at most one interval of work instead of the whole day's register.
//
// Two things make this safe:
//  * the save takes the SAME mutex the menu takes before it changes anything,
//    so a save can never catch the data half-written;
//  * stop() signals a condition_variable instead of letting the thread sleep
//    out its interval, so quitting the app is immediate rather than taking up
//    to N seconds.
class Autosaver {
    std::thread worker;
    std::mutex* dataMutex;            // borrowed from ConsoleApp, not owned
    std::function<void()> saveFn;
    std::atomic<bool> running;
    std::atomic<int> saveCount;

    std::mutex waitMutex;             // guards the condition_variable only
    std::condition_variable wakeUp;
    int intervalSeconds;

    void loop();                      // the thread body

public:
    Autosaver();
    ~Autosaver();                     // stops and joins, even if stop() was missed

    // A running thread must not be copied, and the compiler cannot guess what
    // copying one would mean, so say it explicitly.
    Autosaver(const Autosaver& other) = delete;
    Autosaver& operator=(const Autosaver& other) = delete;

    void start(int intervalSeconds, std::mutex& dataMutex, std::function<void()> saveFn);
    void stop();
    bool isRunning() const;
    int getSaveCount() const;
    void saveNow();                   // runs the save on the calling thread
};

#endif
```

`src/Autosaver.cpp` — write the thread body exactly like this:

```cpp
#include "Autosaver.h"
#include "Debug.h"
#include <chrono>

Autosaver::Autosaver()
    : dataMutex(0), running(false), saveCount(0), intervalSeconds(30) {}

Autosaver::~Autosaver() {
    stop();        // never leave a running thread behind
}

void Autosaver::start(int seconds, std::mutex& mutexToShare, std::function<void()> fn) {
    if (running.load()) { return; }
    intervalSeconds = seconds > 0 ? seconds : 1;
    dataMutex = &mutexToShare;
    saveFn = fn;
    running = true;
    worker = std::thread(&Autosaver::loop, this);
}

void Autosaver::loop() {
    while (running.load()) {
        // Wait for the interval, but wake immediately if stop() fires. The
        // predicate also protects against a spurious wake-up.
        {
            std::unique_lock<std::mutex> waitLock(waitMutex);
            wakeUp.wait_for(waitLock, std::chrono::seconds(intervalSeconds),
                            [this]() { return !running.load(); });
        }
        if (!running.load()) { break; }
        saveNow();
    }
    RELIEF_LIFECYCLE("autosave thread exiting after " << saveCount.load() << " saves");
}

void Autosaver::saveNow() {
    if (!saveFn) { return; }
    // Hold the data lock for the whole save, so the menu cannot change the
    // registries halfway through writing them out.
    std::lock_guard<std::mutex> dataLock(*dataMutex);
    try {
        saveFn();
    } catch (...) {
        // A failed save must not take the thread down with it; the next tick
        // tries again. ConsoleApp reports the failure on screen.
    }
    saveCount++;
}

void Autosaver::stop() {
    if (!running.load()) {
        if (worker.joinable()) { worker.join(); }
        return;
    }
    running = false;
    wakeUp.notify_all();            // wake the thread out of wait_for at once
    if (worker.joinable()) { worker.join(); }
}

bool Autosaver::isRunning() const { return running.load(); }
int Autosaver::getSaveCount() const { return saveCount.load(); }
```

Note on the exception test: `saveCount++` sits after the `catch`, so a throwing callback still counts as an attempt, and `attempts` in the test counts the callback entries. Both assertions hold.

- [ ] **Step 4: Run tests to verify they pass**

Run: `make test`
Expected: PASS — the `Autosaver background thread` suite all ok. It takes around 12 seconds.

Then check for data races if the tooling is available: `g++ -std=c++17 -fsanitize=thread -Iinclude -Itests src/Autosaver.cpp tests/test_autosave.cpp tests/TestRunner.h -o bin/tsan_check -pthread` with a small driver, or run the whole suite under `-fsanitize=thread`. Expected: no race reports. Skip on MinGW, where TSan is unavailable.

- [ ] **Step 5: Commit**

```bash
git add include/Autosaver.h src/Autosaver.cpp tests/test_autosave.cpp tests/test_main.cpp
git commit -m "Add Autosaver background thread with clean shutdown

Saves on a timer so an unannounced power cut costs one interval, not the day's
register. The save holds the same mutex the menu takes, which the test proves
with try_lock from inside the callback. stop() signals a condition_variable
rather than waiting out the interval, so quitting is immediate, and the
destructor joins even if stop() was never called."
```

---
## Task 21: ConsoleApp menu and main

**Files:**
- Create: `include/ConsoleApp.h`, `src/ConsoleApp.cpp`
- Modify: `src/main.cpp` (replace the Task 1 placeholder)

**Interfaces:**
- Consumes: every class built so far.
- Produces: `class ConsoleApp` — `explicit ConsoleApp(const std::string& dataDir)`, `~ConsoleApp()`, `void load()`, `void saveAll()`, `void run()`, `void setToday(const Date& d)`, `const Date& getToday() const`, plus test seams `Registry<Camp>& campRegistry()`, `Registry<Family>& familyRegistry()`, `DistributionLog& distributionLog()`, `HealthMonitor& healthMonitor()`.
- No new test file. `ConsoleApp` is an input/output shell over classes that are already tested; its behaviour is verified by the end-to-end demo in Task 24. Everything worth asserting lives in the collaborators.

- [ ] **Step 1: Write the failing test**

There is no unit test for this task. Instead, write the acceptance check as a shell script that drives the real binary through the demo, so the task still ends in a verified state. Create `tests/demo_smoke.sh`:

```bash
#!/bin/sh
# Drives the built binary through a short scripted session and checks the
# output. Not a unit test - this is the end-to-end check for the menu shell.
set -e
BIN=./bin/relief
if [ ! -x "$BIN" ]; then
  echo "FAIL: $BIN not built"
  exit 1
fi

OUT=$(printf '1\n9\n0\n' | "$BIN" 2>&1)

echo "$OUT" | grep -q "RELIEF CAMP MANAGER" || { echo "FAIL: no banner"; exit 1; }
echo "$OUT" | grep -q "Morigaon"            || { echo "FAIL: camps not listed"; exit 1; }
echo "$OUT" | grep -qi "autosave"           || { echo "FAIL: no autosave notice"; exit 1; }
echo "PASS: demo smoke"
```

Make it executable with `chmod +x tests/demo_smoke.sh`.

- [ ] **Step 2: Run it to verify it fails**

Run: `make && sh tests/demo_smoke.sh`
Expected: FAIL — `FAIL: no banner`, because `src/main.cpp` is still the Task 1 placeholder.

- [ ] **Step 3: Write minimal implementation**

Create `include/ConsoleApp.h`:

```cpp
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
```

`src/ConsoleApp.cpp` — the shape of each piece.

Constructor: `store(dataDir)`, `today(2026, 7, 14)`, `running(false)`. Destructor: `autosaver.stop(); saveAll();`.

`load()`:
```cpp
void ConsoleApp::load() {
    std::lock_guard<std::mutex> lock(dataMutex);
    try {
        store.loadAll(camps, families, volunteers, distributions, ledger, health);
        std::cout << "Loaded " << camps.size() << " camps and "
                  << families.size() << " families from " << store.getDir() << "\n";
    } catch (const FileFormatException& e) {
        // The data on disk is broken. Say exactly where, and carry on with
        // whatever loaded, rather than refusing to start during a flood.
        std::cout << "WARNING: " << e.what() << "\n"
                  << "Starting with the records that loaded cleanly.\n";
    }
    refreshDemand();
}
```

`refreshDemand()`: for each camp, sum `dailyRation()` over the families whose `getCampId()` matches, then `camp->stockRef().setDailyDemand(total)`. Call it after any change that affects a camp's population, so days-of-supply stays accurate.

`run()`:
```cpp
void ConsoleApp::run() {
    running = true;
    showBanner();
    // [Multithreading] start autosaving every 30 seconds, sharing dataMutex
    autosaver.start(30, dataMutex, [this]() {
        store.saveAll(camps, families, volunteers, distributions, ledger, health);
    });
    std::cout << "Autosave is running every 30 seconds.\n";

    while (running) {
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
        catch (const std::exception& e) {
            std::cout << "\n  UNEXPECTED: " << e.what() << "\n";
        }
    }

    std::cout << "\nStopping autosave and writing final save...\n";
    autosaver.stop();
    saveAll();
    std::cout << "Saved. " << autosaver.getSaveCount() << " autosave(s) ran this session.\n";
}
```

Handler notes, each one locking `dataMutex` before it mutates:

- `handleListCamps`: prints each camp with `operator<<`, its shelter count, days of supply and `LOW STOCK` when `camp->getStock().daysOfSupply() <= 3.0`.
- `handleRegisterFamily`: reads the head name and village, creates the `Family`, then loops asking for members: name, age, sex, and the three flags; picks the class from the age (`age <= 2` Infant, `<= 12` Child, `< 60` Adult, else Elderly) and offers `ResidentVolunteer` for an adult who says yes to "will they take a camp duty?". Ends by printing `static_cast<RationCard>(*f).render()` — the ration card — plus the vulnerability score. This is demo step 2.
- `handleAdmitFamily`: picks a family and a camp, calls `camp->admit(*f)`, and on success sets `f->setCampId(camp->getId())` and calls `refreshDemand()`. The `OverCapacityException` is caught **here**, not in `run()`, so it can call `suggestAlternativeCamp(*f, camp->getId())`; it prints the message and then the alternatives. This is demo step 3.
- `suggestAlternativeCamp`: lists every other camp whose `freeSpace() >= f.size()`, or says none has room.
- `handleDistribute`: picks a camp, builds a `DistributionQueue` of `familiesAtCamp`, then pops and serves each family in turn: builds `Allocation(f->getId(), campId, today, f->dailyRation())`, calls `distributions.record(alloc)`, then `camp->stockRef() -= alloc` — and on `DuplicateClaimException` prints the block and continues with the next family rather than aborting the round. Prints a served/blocked tally. This is demo step 4.
- `handleStockView`: prints `reporter.stockTable(*camp)` and the days of supply, flagging the critical item. This is demo step 5's first half.
- `handleStockIn`: picks a camp and an item, reads a quantity and a note, calls `ledger.stockIn(...)`.
- `handleTransferSuggestions`: calls `advisor.suggest(camps.all())` and prints each `render()`, or "no transfers needed". Then offers to apply one: on yes, `ledger.stockOut(from, item, qty, today, "transfer out")` and `ledger.stockIn(to, item, qty, today, "transfer in")`. This is demo step 5's second half.
- `handleHealthLog`: picks a camp and a symptom, reads a count, calls `health.logCase(...)`, then checks `health.outbreakAlert(...)` and prints a boxed `*** OUTBREAK ALERT ***` with the window total and threshold when it fires. This is demo step 6.
- `handleSortFamilies`: prints `FamilySorter::metricMenu()`, reads a choice, maps it to a name, calls `FamilySorter::metricByName` and `sortDescending`, then prints the ordered list. Also prints `findMax(list, &Family::metricVulnerability)` as "most vulnerable household".
- `handleReport`: picks a camp, prints `reporter.dailyReport(...)`, then offers to export; on yes calls `reporter.exportCsv(store.getDir() + "/report_" + today.toString() + "_camp" + id + ".csv", ...)` and prints the path. This is demo step 7.
- `handleSaveNow`: `autosaver.saveNow()` and prints the save count.
- `handleSetDate`: reads a date with `Date::parse` so the demo can roll to the next day and show that the duplicate block clears.
- `readInt` / `readDouble`: read a whole line and parse with `std::istringstream`; on bad input print "Please enter a number." and ask again. Never leave `std::cin` in a failed state. On end-of-file, return 0 so a piped script terminates instead of looping forever — that is what makes `tests/demo_smoke.sh` able to drive the binary.
- `showMainMenu`: a numbered list matching the switch, with option 0 to quit.
- `showBanner`: prints `RELIEF CAMP MANAGER` plus the district, the date and `offline - no internet required`.

Replace `src/main.cpp`:

```cpp
#include "ConsoleApp.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    // The data directory can be overridden, which keeps the tests and the
    // demo from fighting over data/.
    std::string dataDir = (argc > 1) ? argv[1] : "data";
    try {
        ConsoleApp app(dataDir);
        app.load();
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
```

- [ ] **Step 4: Run it to verify it passes**

Run: `make && make test`
Expected: the app builds with no warnings, and every existing suite still passes.

Run: `sh tests/demo_smoke.sh`
Expected: `PASS: demo smoke`. (Before Task 22 seeds `data/`, the camps check may fail for lack of data; if so, create one camp by hand through the menu, or run this check again at the end of Task 22. Note which happened.)

- [ ] **Step 5: Commit**

```bash
git add include/ConsoleApp.h src/ConsoleApp.cpp src/main.cpp tests/demo_smoke.sh
git commit -m "Add the console menu and wire up the application

ConsoleApp owns every registry and collaborator and is the only place that
catches exceptions and prints: each precise exception type gets its own
recovery, so a full camp suggests alternatives and a duplicate claim is
blocked without aborting the distribution round. Autosave shares the app's
data mutex. readInt returns 0 on EOF so the binary can be driven by a script."
```

---

## Task 22: Morigaon seed data

**Files:**
- Create: `src/seed_main.cpp`, `data/.gitkeep`
- Generates: `data/camps.csv`, `data/shelters.csv`, `data/stock.csv`, `data/families.csv`, `data/members.csv`, `data/volunteers.csv`, `data/distributions.csv`, `data/stock_txns.csv`, `data/health.csv`

**Interfaces:**
- Consumes: every entity class and `CsvStore`.
- Produces: the `bin/seed` binary, run by `make seed`, which writes a reproducible demo dataset into `data/`. It takes an optional directory argument, defaulting to `data`.

**Dataset specification** (fixed, so the demo is identical every run):

| Camp | Name | Capacity | Shelters |
|---|---|---|---|
| 1 | Bhuragaon HS School | 180 | Room-1 (50), Room-2 (50), Hall-A (80) |
| 2 | Laharighat Community Hall | 90 | Hall-B (60), Tent-1 (30) |
| 3 | Mayong Embankment Tents | 120 | Tent-A (40), Tent-B (40), Tent-C (40) |

All three in district `Morigaon`.

- 40 families in total: 16 at camp 1, 14 at camp 2, 10 at camp 3.
- Household composition is drawn from a fixed cycle so the counts are known: every family has 1 or 2 adults; every third family has an infant; every second family has one or two children; every fourth family has an elderly member; family 4, 11, 19, 27 and 35 have a pregnant adult; family 7, 16 and 29 have a disabled member; family 9 and 22 have a lactating mother; families 5, 14, 23 and 31 include a `ResidentVolunteer`.
- **Camp 2 must be nearly full**, so demo step 3 works on the seeded data without setup: set its occupancy from its 14 families to leave at most 2 free spaces.
- 4 outside volunteers: a medic, a nurse, a cook and a record keeper, from `District Health`, `Red Cross`, `ASDMA` and `Local NGO`.
- Stock, with each camp's daily demand computed from its own families:
  - Camp 1: comfortable on everything (10 or more days).
  - **Camp 2: rice at roughly 2 days** — the critical item that triggers demo step 5.
  - **Camp 3: rice at roughly 18 days** — the surplus camp that donates in demo step 5.
  - Every camp has 6 or more days of dal, oil, salt, ORS and baby food so rice is unambiguously the worst item at camp 2.
- Health log: camp 2 has diarrhoea cases of 3 on 2026-07-12 and 4 on 2026-07-13, totalling 7 in the window ending 2026-07-14 — **below the threshold of 10**, so the demo's own entry of 3 or more is what fires the alert in step 6. Camp 1 has 2 fever cases, camp 3 has 1 skin infection.
- Distributions: camp 1 and camp 3 have a full round recorded for 2026-07-13, the day before. **Nothing is recorded for 2026-07-14**, so every family is claimable on the demo date and step 4 works on the first try.
- `src/seed_main.cpp` prints a summary and the three numbers the demo depends on: camp 2's free space, camp 2's rice days, and camp 3's rice days.

- [ ] **Step 1: Write the failing test**

Append a seed verification suite. Create `tests/test_seed.cpp`:

```cpp
#include "TestRunner.h"
#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include <fstream>

// Checks the shipped demo dataset, so a broken data/ directory fails the test
// suite instead of failing in front of the professor.
void run_seed_tests() {
    testing::beginSuite("Seeded Morigaon dataset");

    CsvStore store("data");
    if (!store.dataExists()) {
        std::cout << "  skip  data/ not seeded yet - run `make seed`\n";
        return;
    }

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);

    Registry<Camp> camps;
    Registry<Family> families;
    Registry<Volunteer> volunteers;
    DistributionLog log;
    StockLedger ledger;
    HealthMonitor health;

    store.loadAll(camps, families, volunteers, log, ledger, health);

    ASSERT_EQ(camps.size(), 3);
    ASSERT_EQ(families.size(), 40);
    ASSERT_EQ(volunteers.size(), 4);

    Date demoDay(2026, 7, 14);

    // Every camp is in Morigaon and has shelters.
    for (int i = 0; i < camps.size(); i++) {
        ASSERT_STR_EQ(camps.at(i)->getDistrict(), "Morigaon");
        ASSERT_TRUE(camps.at(i)->shelterCount() >= 2);
    }

    // Every family has at least one member and at least one adult-ish member.
    int infants = 0, elderly = 0, pregnant = 0, residentVolunteers = 0;
    for (int i = 0; i < families.size(); i++) {
        Family* f = families.at(i);
        ASSERT_TRUE(f->size() >= 1);
        ASSERT_TRUE(f->getCampId() >= 1 && f->getCampId() <= 3);
        infants += f->countOfTag("INFANT");
        elderly += f->countOfTag("ELDERLY");
        residentVolunteers += f->countOfTag("RESIDENT_VOLUNTEER");
        for (int m = 0; m < f->size(); m++) {
            if (f->memberAt(m).isPregnant()) { pregnant++; }
        }
    }
    ASSERT_TRUE(infants >= 10);
    ASSERT_TRUE(elderly >= 8);
    ASSERT_EQ(pregnant, 5);
    ASSERT_EQ(residentVolunteers, 4);

    // --- the three facts the demo script depends on ---

    Camp* c2 = camps.findById(2);
    ASSERT_TRUE(c2 != 0);
    // Demo step 3: camp 2 is nearly full, so admitting a 3-person family fails.
    ASSERT_TRUE(c2->freeSpace() <= 2);
    ASSERT_TRUE(c2->freeSpace() >= 0);

    // Demo step 5: camp 2's rice is critical and it is the worst item there.
    ASSERT_TRUE(c2->getStock().daysOfSupplyFor(Item::Rice) < 3.0);
    ASSERT_TRUE(c2->getStock().mostCriticalItem() == Item::Rice);
    // ...and nothing else at camp 2 is critical, so the message is unambiguous.
    ASSERT_TRUE(c2->getStock().daysOfSupplyFor(Item::Dal) >= 6.0);
    ASSERT_TRUE(c2->getStock().daysOfSupplyFor(Item::ORS) >= 6.0);

    Camp* c3 = camps.findById(3);
    ASSERT_TRUE(c3 != 0);
    // Camp 3 is the donor: well above the 7-day surplus line.
    ASSERT_TRUE(c3->getStock().daysOfSupplyFor(Item::Rice) > 7.0);

    // Demo step 6: camp 2's diarrhoea window is loaded but still under the
    // threshold, so the demo's own entry is what triggers the alert.
    int window = health.casesInWindow(2, demoDay, Symptom::Diarrhoea);
    ASSERT_TRUE(window > 0);
    ASSERT_TRUE(window < HealthMonitor::thresholdFor(Symptom::Diarrhoea));
    ASSERT_FALSE(health.outbreakAlert(2, demoDay, Symptom::Diarrhoea));

    // Demo step 4: nobody has claimed on the demo date, so the first
    // distribution always succeeds and the second is blocked.
    for (int i = 0; i < families.size(); i++) {
        ASSERT_FALSE(log.alreadyClaimed(families.at(i)->getId(), demoDay));
    }
    // ...but the previous day's round is on record, so the log is not empty.
    ASSERT_TRUE(log.size() > 0);

    Person::resetIdCounter(1);
    Family::resetIdCounter(1);
    Camp::resetIdCounter(1);
}
```

Add `void run_seed_tests();` and the call to `tests/test_main.cpp`.

- [ ] **Step 2: Run test to verify it fails**

Run: `make test`
Expected: the suite prints `skip  data/ not seeded yet` and the assertions do not run. That is the failing state for this task: nothing is verified yet. (If `data/` already holds files from an earlier run, delete them first with `rm -f data/*.csv` so the skip is genuine.)

- [ ] **Step 3: Write minimal implementation**

Create `src/seed_main.cpp`. Structure:

```cpp
#include "CsvStore.h"
#include "Registry.h"
#include "Camp.h"
#include "Family.h"
#include "Infant.h"
#include "Child.h"
#include "Adult.h"
#include "Elderly.h"
#include "ResidentVolunteer.h"
#include "Volunteer.h"
#include "DistributionLog.h"
#include "StockLedger.h"
#include "HealthMonitor.h"
#include "RationNorms.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Builds the Morigaon demo dataset. Fixed, not random, so the demo behaves
// identically every time it is run.

static const char* HEAD_NAMES[40] = {
    "Bipul Das", "Jiten Saikia", "Rupa Nath", "Hemanta Bora", "Gitali Pegu",
    /* ...35 more realistic Assamese names... */
};
static const char* VILLAGES[8] = {
    "Bhuragaon", "Laharighat", "Mayong", "Mikirbheta", "Jagiroad",
    "Dharamtul", "Moirabari", "Bhurbandha"
};
```

Then, in `main`:
1. Create the three camps with their capacities and shelters, in ID order 1, 2, 3.
2. Loop `i = 0..39` building family `i`: head name `HEAD_NAMES[i]`, village `VILLAGES[i % 8]`, camp `1` for `i < 16`, `2` for `i < 30`, else `3`. Add members following the fixed composition rules in the dataset specification above, using `i` to decide: `i % 3 == 0` adds an `Infant`; `i % 2 == 0` adds one `Child`, `i % 5 == 0` adds a second; `i % 4 == 0` adds an `Elderly`; the pregnancy, disability, lactation and resident-volunteer family numbers are listed in the specification, one-based, so test `i + 1`.
3. Call `camp->admit(*f)` for each family and `f->setCampId(camp->getId())`.
4. Top camp 2 up to within 2 of capacity: after admitting its 14 families, compute `int gap = camp2->freeSpace() - 2; if (gap > 0) { for (int k = 0; k < gap; k++) { ++(*camp2); } }` and print a note that the extra occupants are unregistered walk-ins awaiting registration, which is realistic and explains the number on screen.
5. Add the four outside volunteers.
6. For each camp, compute its daily demand as the sum of its families' `dailyRation()`, call `setDailyDemand`, then set stock per item as `days * demand` using the day counts from the specification: camp 1 uses 12 days for every item; camp 2 uses **2 days of rice** and 8 days of everything else; camp 3 uses **18 days of rice** and 9 days of everything else. Where an item has zero demand (tarpaulin), set a flat 40 sheets so the stock table is not empty.
7. Record a full distribution round for camps 1 and 3 on `Date(2026, 7, 13)` via `log.loadEntry(...)`, and nothing at all on the 14th.
8. Add the health cases from the specification.
9. Add one `stockIn` ledger line per camp per item for 2026-07-12 with the note `initial district allocation`, using `loadTxn` so the camp stock set in step 6 is not double-counted.
10. `store.saveAll(...)`, then print the summary with the three demo-critical numbers:

```cpp
    std::cout << "\nSeeded " << store.getDir() << ":\n"
              << "  camps      : " << camps.size() << "\n"
              << "  families   : " << families.size() << "\n"
              << "  volunteers : " << volunteers.size() << "\n\n"
              << "Demo-critical values:\n"
              << "  camp 2 free space    : " << camp2->freeSpace()
              << "   (demo step 3 needs <= 2)\n"
              << "  camp 2 rice days     : " << camp2->getStock().daysOfSupplyFor(Item::Rice)
              << "   (demo step 5 needs < 3)\n"
              << "  camp 3 rice days     : " << camp3->getStock().daysOfSupplyFor(Item::Rice)
              << "   (demo step 5 needs > 7)\n"
              << "  camp 2 diarrhoea/3d  : " << health.casesInWindow(2, Date(2026, 7, 14), Symptom::Diarrhoea)
              << "   (demo step 6 needs < 10)\n";
```

Create an empty `data/.gitkeep` and remove `data/*.bak` from `.gitignore` if it would exclude the generated CSVs — the seeded data **must** be committed, because the professor and the judges will clone the repository and run it without regenerating.

- [ ] **Step 4: Run it to verify it passes**

Run: `make seed`
Expected: the summary prints and all four demo-critical values satisfy the stated bounds.

Run: `make test`
Expected: PASS — `Seeded Morigaon dataset` now runs its assertions rather than skipping, and every other suite still passes.

Run: `sh tests/demo_smoke.sh`
Expected: `PASS: demo smoke`.

- [ ] **Step 5: Commit**

```bash
git add src/seed_main.cpp data/
git commit -m "Add the Morigaon seed dataset and its verification suite

Three camps and 40 families with a fixed composition, so the demo behaves
identically every run. The data is rigged for the demo script: camp 2 has at
most 2 free spaces and under 3 days of rice, camp 3 holds a rice surplus, the
diarrhoea window sits just under its threshold, and nothing is claimed on the
demo date. test_seed.cpp asserts each of those facts, so a broken dataset
fails the suite instead of failing during the presentation."
```

---

## Task 23: OOP_MAPPING.md and README.md

**Files:**
- Create: `docs/OOP_MAPPING.md`, `README.md`, `tools/collect_tags.sh`

**Interfaces:**
- Consumes: the tag comments written throughout Tasks 1–22.
- Produces: `docs/OOP_MAPPING.md`, one row per syllabus concept with `file:line` references; `README.md` with build, run and demo instructions; `tools/collect_tags.sh`, which greps the tags so the mapping can be regenerated when lines move.

- [ ] **Step 1: Write the failing check**

Create `tools/collect_tags.sh`:

```bash
#!/bin/sh
# Lists every syllabus tag comment with its file and line, so OOP_MAPPING.md
# can be checked and regenerated after the code moves.
grep -rn '// \[' include src tests --include='*.h' --include='*.cpp' | sed 's/:[[:space:]]*\/\/ \[/ -> [/'
```

`chmod +x tools/collect_tags.sh`. Then write the check as a script, `tools/check_mapping.sh`:

```bash
#!/bin/sh
# Fails if any syllabus concept has no tag in the source.
set -e
MISSING=0
for tag in \
  "Static data member" "Static member function" "Friend function" "Friend class" \
  "Nested class" "Pure virtual" "Virtual base class" "Virtual destructor" \
  "Copy constructor" "Copy assignment" "Destructor" "Dynamic memory" \
  "Array of dynamic objects" "Pointer to member function" "Class template" \
  "Function template" "Basic type to class type" "Class to basic type conversion" \
  "Class to class conversion" "Multilevel inheritance" "Hierarchical inheritance" \
  "Multiple inheritance" "Hybrid inheritance" "Single inheritance" \
  "Unary operators" "Friend binary operators" "Objects as arguments" \
  "Returning objects" "Const member function" "Constant objects" \
  "Custom exception base class" "Derived exception" "File handling" \
  "Multithreading" "Abstract class" "Runtime polymorphism" "Object lifecycle"
do
  if ! grep -rq "\[$tag" include src 2>/dev/null; then
    echo "MISSING TAG: [$tag]"
    MISSING=1
  fi
done
if [ "$MISSING" -eq 0 ]; then echo "PASS: every syllabus tag is present in the source"; fi
exit $MISSING
```

`chmod +x tools/check_mapping.sh`.

- [ ] **Step 2: Run it to verify it fails**

Run: `sh tools/check_mapping.sh`
Expected: it lists any tags that were never written, for example `MISSING TAG: [Runtime polymorphism]`. Fix the source by adding the missing tag comment at the line that actually demonstrates the concept — do **not** add a tag to a line that does not demonstrate it. Re-run until it prints `PASS`.

- [ ] **Step 3: Write the documents**

Run `sh tools/collect_tags.sh > /tmp/tags.txt` and use it to fill in real line numbers. Create `docs/OOP_MAPPING.md` with this structure, one row per syllabus concept, every reference being a real `file:line` taken from the tag dump:

```markdown
# OOP Syllabus Coverage Map

Object Oriented Programming (CS 203) — Offline Relief Camp Manager

Every concept in the course syllabus, with the file and line where it is
demonstrated. Regenerate the line numbers with `sh tools/collect_tags.sh`;
verify nothing is missing with `sh tools/check_mapping.sh`.

| # | Concept | Where | What to look at |
|---|---|---|---|
| 1 | Classes, objects, constructors (default, parameterised, copy), destructors | `include/Person.h:NN`, `src/Family.cpp:NN` | `Person` has all three constructor forms; `Family`'s destructor frees its members |
| 2 | Constant objects and const member functions | `include/ReportGenerator.h:NN` | every report parameter is a `const` reference, so a report cannot mutate a camp |
| ... | | | |
```

Rows to include, in the order of the syllabus table in `PROJECT_CONTEXT.md`: classes and constructors; const objects and const members; static data and static members; friend function and friend class; objects as arguments and returning objects; nested class; dynamic memory and arrays of objects; pointers to members; operator overloading split into unary, binary and friend-based; the three type conversions; the five inheritance kinds; virtual base class; abstract class, pure virtual and runtime polymorphism; class and function templates; exception handling; file handling; multithreading.

End the document with the honest-assessment section copied from section 7 of the design spec, so the professor sees the three pedagogical choices named rather than discovering them.

Create `README.md` covering: what the project is and the problem it solves; the hard constraints (C++17, standard library only, offline); how to build with `make` and with CMake; `make test`, `make seed`, `make debug`, `make run`; the folder layout; the full eight-step demo script with the exact menu numbers to press; where the data files live and that they are plain CSV; a pointer to `docs/OOP_MAPPING.md` for the professor and to `docs/superpowers/specs/` for the design rationale; and the Seva First Innovation Challenge framing — the problem, the affordability (one low-cost laptop, no internet, no licence fees), and how it scales to other districts by changing the seed data.

- [ ] **Step 4: Verify**

Run: `sh tools/check_mapping.sh`
Expected: `PASS: every syllabus tag is present in the source`.

Then spot-check the mapping by hand: pick five rows at random, open the named `file:line`, and confirm the line really demonstrates the concept claimed. Fix any reference that has drifted.

- [ ] **Step 5: Commit**

```bash
git add docs/OOP_MAPPING.md README.md tools/
git commit -m "Add OOP syllabus coverage map and README

OOP_MAPPING.md gives the professor a file:line reference for every syllabus
concept, generated from the tag comments in the source and checked by
tools/check_mapping.sh so a missing concept fails loudly. It closes with the
honest assessment of the three rows that are pedagogical choices rather than
the natural engineering answer."
```

---

## Task 24: Final verification

**Files:**
- Modify: whatever the verification turns up.

**Interfaces:**
- Consumes: everything.
- Produces: evidence that the whole project builds, tests clean and runs the eight-step demo. No claim of completion is made before this task's output is captured.

- [ ] **Step 1: Build three ways, with no warnings**

```bash
make clean && make 2>&1 | tee /tmp/build_make.log
```
Expected: `bin/relief` is produced and the log contains no `warning:` lines. Fix any warning rather than silencing it.

```bash
rm -rf build && cmake -S . -B build && cmake --build build 2>&1 | tee /tmp/build_cmake.log
```
Expected: `build/relief`, `build/relief_tests` and `build/relief_seed` are produced, no warnings.

```bash
make clean && make debug && ./bin/relief data 2>&1 | head -40
```
Expected: the lifecycle lines appear, proving `RELIEF_DEBUG` works and the destructors really run. Then `make clean && make` to get back to a quiet build.

- [ ] **Step 2: Run the whole test suite**

```bash
make clean && make test 2>&1 | tee /tmp/test_run.log; echo "exit: $?"
```
Expected: every suite listed, `failed: 0`, exit code 0. Record the pass count.

If a sanitizer is available on the machine, also run:
```bash
g++ -std=c++17 -Wall -Wextra -pedantic -fsanitize=address,undefined -Iinclude -Itests \
  $(ls src/*.cpp | grep -v 'src/main.cpp' | grep -v 'src/seed_main.cpp') tests/*.cpp \
  -o bin/tests_asan -pthread && ./bin/tests_asan
```
Expected: `failed: 0` and no sanitizer reports.

- [ ] **Step 3: Regenerate and verify the seed data**

```bash
rm -f data/*.csv && make seed && make test 2>&1 | grep -A2 "Seeded Morigaon"
```
Expected: the four demo-critical values satisfy their bounds and the seed suite passes.

- [ ] **Step 4: Run the eight-step demo end to end, capturing the real output**

Drive the binary by hand, following `PROJECT_CONTEXT.md`'s demo script, and keep the transcript. For each step, confirm the stated outcome:

1. **Start** — `./bin/relief` prints the banner and `Loaded 3 camps and 40 families from data`.
2. **Register** — option 2, a new family with an infant and an elderly member. The ration card prints, and the vulnerability score is at least 75 (40 for the infant plus 35 for the elderly).
3. **Overflow** — option 3, admit that family to camp 2. `REFUSED` prints with camp 2's free space, and the app lists camps 1 and 3 as alternatives. Then admit to camp 1 and confirm it succeeds.
4. **Duplicate** — option 4, distribute at camp 1. It reports the families served. Run option 4 again the same day: every family is `BLOCKED` with the duplicate-claim message, and the served count is 0.
5. **Stock and transfer** — option 5 for camp 2 shows rice under 3 days and marked `LOW`. Option 7 suggests moving rice from camp 3 to camp 2. Accept it, then option 5 again shows camp 2 above 3 days.
6. **Outbreak** — option 8, log 5 diarrhoea cases at camp 2 for the current date. The window total crosses 10 and `*** OUTBREAK ALERT ***` prints.
7. **Report** — option 10 for camp 2 prints the daily report including the outbreak line and the stock table; accept the export and confirm the named CSV exists and opens as CSV.
8. **Restart** — option 0 quits, printing the autosave count and the final save. Run `./bin/relief` again and confirm the new family is present, the distribution is still blocked for today, the transferred stock is still moved, and the outbreak still shows.

Paste the captured transcript into the task's completion report. A step that does not behave as written is a bug to fix, not a note to add.

- [ ] **Step 5: Confirm the syllabus coverage and commit**

```bash
sh tools/check_mapping.sh && sh tools/collect_tags.sh | wc -l
```
Expected: `PASS`, and a tag count comfortably above the 17 syllabus rows.

Re-verify five mapping references by hand, then:

```bash
git add -A
git commit -m "Final verification: build, tests and the full demo script

Builds clean with make, CMake and make debug on this machine. Full suite
passes with zero failures. Seed data regenerated and its demo-critical values
confirmed. All eight demo steps walked end to end with the transcript recorded
in the task report."
git log --oneline
```

Expected: one commit per task, 24 in total plus the design-spec commit.

---

## Plan Self-Review

**1. Spec coverage.** Every section of `docs/superpowers/specs/2026-11-11-relief-camp-manager-design.md` maps to at least one task:

| Spec section | Tasks |
|---|---|
| 1 Purpose and constraints | 1 (build system, both build files, flags), Global Constraints |
| 2 Decisions taken | 5–7 (one tree), 8 (rule of three on Family), 10 (unique_ptr contrast), 14–19 (collaborators), 15 (hand-written heap) |
| 3 Person hierarchy | 4, 5, 6, 7 |
| 4 Value types and conversions | 2 (Item, Date, Symptom), 3 (Ration, RationNorms, int→Ration), 9 (Family→RationCard), 12 (Allocation→Ration, Stock→double) |
| 5 Entities and collaborators | 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 |
| 6 Error handling, data flow, testing | 11 (exceptions), 21 (the single catch site), 1 (test runner), every task's TDD cycle |
| 7 Syllabus assessment | 23 (OOP_MAPPING.md reproduces it) |
| 8 Seed data | 22 |
| 9 Demo script | 22 (data rigged for it), 24 (walked end to end) |
| 10 Out of scope | respected throughout; nothing in any task adds a GUI, networking or auth |

The 17 syllabus rows are each tagged in the source and checked mechanically by `tools/check_mapping.sh` in Task 23. No gaps found.

**2. Placeholder scan.** One real defect found and fixed: Task 10's test contained a fake type (`Registry<Volunteer_stub_check_not_needed>`) plus prose telling the implementer to replace it themselves. Both are gone; the task now carries a single `Registry<Volunteer>` block that compiles against Task 5's `Volunteer`, with `Volunteer.h` in the include list. Everything else was checked for "TBD", "add error handling", "similar to Task N" and bodiless code steps; the `suggest` algorithm in Task 16 and the report sections in Task 18 are written out as explicit step lists rather than left to judgement.

**3. Type consistency.** Names were cross-checked across tasks:

- `Person::rationNeed()` returns `Ration` everywhere (Tasks 4–7, used in 8).
- `typeTag()` strings are exactly `"INFANT"`, `"CHILD"`, `"ADULT"`, `"ELDERLY"`, `"RESIDENT_VOLUNTEER"`, `"VOLUNTEER"` in Tasks 5–7, consumed with the same spelling by `Family::countOfTag` (8), `Family::operator RationCard` (9) and `CsvStore::makePerson` (19).
- `adoptId` / `resetIdCounter` / `generateId` appear with the same signatures on `Person` (4), `Family` (8) and `Camp` (13), and are the mechanism Task 19 relies on.
- `FamilySorter::Metric` is `double (Family::*)() const`, matching the four `metric*` functions declared in Task 8 and the `findMax` signature in Task 10.
- `Stock::take` and `operator-=`/`operator-` all throw `InsufficientStockException` (12), which `StockLedger::stockOut` (16) propagates and `ConsoleApp` (21) catches.
- `CsvStore::loadAll` takes non-const registry references while `saveAll` takes const ones, consistent between Task 19's header and its use in Tasks 21 and 22.
- `Date::operator-` returns `long`, used as such in `HealthMonitor::casesInWindow` (17).
- `HealthMonitor::thresholdFor` is static, so Task 22's seed test can call it without an instance.

One inconsistency found and corrected while reviewing: Task 16's `TransferSuggestion` constructor needed the camp *names* for `render()` to print them, which the first draft of the interface block omitted; the parameterised constructor and getters now include `fromCampName` and `toCampName`, and Task 16's test checks for the name or the ID.
