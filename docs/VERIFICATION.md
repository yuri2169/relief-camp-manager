# Verification transcript (Task 24)

Everything below was run on the final committed tree, on one machine, and the
output is pasted from the real runs. Where a block is trimmed the text says so.
Nothing here is claimed from memory.

## Result summary

| # | Check | Result |
|---|-------|--------|
| 1 | `make clean && make` (Apple clang 21) | PASS, `bin/relief` built, 0 warnings |
| 2 | `make clean && make test` (Apple clang 21) | PASS, `passed: 1923   failed: 0`, 0 warnings, about 22 s wall |
| 3 | `make debug`, then a normal build | PASS, 1130 `[lifecycle]` lines in a debug run, 0 in a normal run |
| 4 | CMake configure, build, run tests | PASS, 3 targets built, 0 warnings, `passed: 1923   failed: 0` |
| 5 | GNU g++ 15.2.0 build and suite | PASS after a 2-line fix (3 warnings found first, see below) |
| 6 | `make seed` determinism | PASS, all 9 data files byte-identical, `git status data/` empty |
| 7 | `bash tests/demo_smoke.sh` | PASS |
| 8 | Eight-step demo, real binary, piped input | PASS, all 8 steps (details below) |

Compilers exercised: **Apple clang 21.0.0** (`c++`/`g++` on this Mac) and
**Homebrew GCC 15.2.0** (`g++-15`). **MinGW / Windows was not tested**, and no
Linux machine was used either.

Honest notes:

* GNU g++ found 3 `-Wdeprecated-copy` warnings that clang did not. They were
  fixed (see section 5). All builds were re-run after the fix.
* Step 5 of the demo: after the transfer rice shows exactly `3.0 days` and the
  `LOW` flag is gone. That is "no longer low", not "strictly above 3".
* Not verified: AddressSanitizer / UBSan run (not requested for this task),
  CMake build under g++-15, behaviour with a real clock across a 30-second
  autosave tick during the demo (the demo finishes inside one interval, so the
  session printed `0 autosave(s)`; the autosave thread itself is covered by the
  unit suite, which sleeps about 12 s by design).
* After the g++ fix, the CMake path was not re-run (the change is two
  defaulted member functions in two headers; the Makefile builds with both
  compilers were re-run and pass).

## 1. `make clean && make` (Apple clang)

Command: `make clean && make`   (exit 0, 0 lines containing "warning")

```text
rm -rf build bin
mkdir -p bin
c++ -std=c++17 -Wall -Wextra -pedantic -Iinclude <all src/*.cpp ...> -o ... -pthread   [command line shortened]
```

## 2. `make clean && make test` (Apple clang)

Command: `make clean && make test`   (exit 0, 0 lines containing "warning").
The 1923 passing assertion lines (`  ok ...`) are omitted here; every
suite heading and the summary are kept. Wall time for build plus run:
`9.85s user 1.25s system 50% cpu 22.090 total` (measured on an earlier identical run).

```text
rm -rf build bin
mkdir -p bin
c++ -std=c++17 -Wall -Wextra -pedantic -Iinclude <all src/*.cpp ...> -o ... -pthread   [command line shortened]
./bin/tests
Relief Camp Manager - test suite

-- test runner self-test --

-- Item, Symptom and Date --

-- Ration and RationNorms --

-- Person abstract base --

-- Person hierarchy: virtual bases --

-- Person hierarchy: age bands --

-- Person hierarchy: multilevel and the diamond --

-- Family and the rule of three --

-- Family to RationCard conversion --

-- Registry<T> and findMax --

-- Custom exception hierarchy --

-- Stock, Allocation and days of supply --

-- Camp and the nested Shelter class --

-- DistributionLog and duplicate claims --

-- DistributionQueue and FamilySorter --

-- StockLedger history --

-- StockLedger and TransferAdvisor --

-- HealthMonitor and the outbreak alert --

-- ReportGenerator --

-- Csv helpers and CsvStore round trip --

-- Autosaver background thread --

-- Seeded Morigaon dataset --

=========================================
  passed: 1923   failed: 0
=========================================
```

## 3. `make debug`: lifecycle messages appear only in debug builds

Command: `make clean && make debug`, then `echo 0 | ./bin/relief <copy of data>`.
Build: exit 0, 0 warnings. The run printed **1130** `[lifecycle]` lines.
First 6 and last 6 shown (middle trimmed):

```text
[lifecycle] Camp #1 created
[lifecycle] Camp #2 created
[lifecycle] Camp #3 created
[lifecycle] Family created   #1
[lifecycle] Family created   #2
[lifecycle] Family created   #3
... [trimmed] ...
[lifecycle] Adult destroyed #5
[lifecycle] Resident destroyed #5
[lifecycle] Person destroyed #5 Sanjib Das
[lifecycle] Camp #3 destroyed
[lifecycle] Camp #2 destroyed
[lifecycle] Camp #1 destroyed
```

Then `make clean && make` and the same run. Count of `lifecycle` in the output: **0**.
Full output of that normal run:

```text
Loaded 3 camps and 40 families from /tmp/d3b

==========================================
  RELIEF CAMP MANAGER
  Morigaon district
  Date: 2026-07-14
  offline - no internet required
==========================================
Autosave is running every 30 seconds.

--- MAIN MENU (2026-07-14) ---
 1. List camps
 2. Register a family
 3. Admit a family to a camp
 4. Distribute rations
 5. View stock
 6. Receive stock
 7. Transfer suggestions
 8. Log a health case
 9. Sort families
10. Daily report
11. Save now
12. Set today's date
13. Add a camp
 0. Quit
Choose: 
Stopping autosave and writing final save...
Saved. 0 autosave(s) ran this session.
```

## 4. CMake path

Command: `cmake -S . -B build/cmake && cmake --build build/cmake`, then
`./build/cmake/relief_tests`. `build/cmake` was removed afterwards.
CMake adds `-Wall -Wextra -pedantic` (CMakeLists.txt line 11). Exit 0, 0 warnings.
The per-file `Building CXX object` lines are trimmed (they repeat 3 times, once per target).

```text
-- The CXX compiler identification is AppleClang 21.0.0.21000334
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Success
-- Found Threads: TRUE
-- Configuring done (0.5s)
-- Generating done (0.0s)
-- Build files have been written to: <repo>/build/cmake
[ 27%] Linking CXX executable relief
[ 27%] Built target relief
[ 72%] Linking CXX executable relief_tests
[ 72%] Built target relief_tests
[100%] Linking CXX executable relief_seed
[100%] Built target relief_seed
--- ./build/cmake/relief_tests (last lines) ---

=========================================
  passed: 1923   failed: 0
=========================================
```

Produced: `relief`, `relief_tests`, `relief_seed`.

## 5. GNU g++ 15.2.0

`Makefile` takes `CXX` (`CXX ?= g++`), so: `make clean && make CXX=g++-15` and
`make clean && make test CXX=g++-15`. `g++-15 --version`:
`g++-15 (Homebrew GCC 15.2.0_1) 15.2.0`.

### 5a. First run: 3 warnings, verbatim

The build exited 0 but printed these (the test build printed the same three):

```text
rm -rf build bin
mkdir -p bin
g++-15 -std=c++17 -Wall -Wextra -pedantic -Iinclude <all src/*.cpp ...> -o ... -pthread   [command line shortened]
src/ConsoleApp.cpp: In member function 'void ConsoleApp::setToday(const Date&)':
src/ConsoleApp.cpp:732:52: warning: implicitly-declared 'constexpr Date& Date::operator=(const Date&)' is deprecated [-Wdeprecated-copy]
  732 | void ConsoleApp::setToday(const Date& d) { today = d; }
      |                                                    ^
In file included from include/Allocation.h:5,
                 from include/DistributionLog.h:4,
                 from include/ConsoleApp.h:8,
                 from src/ConsoleApp.cpp:1:
include/Date.h:16:5: note: because 'Date' has user-provided 'Date::Date(const Date&)'
   16 |     Date(const Date& other);               // copy constructor
      |     ^~~~
src/CsvStore.cpp: In function 'Date parseDate(const std::string&, const std::string&, int)':
src/CsvStore.cpp:120:32: warning: implicitly-declared 'constexpr Date& Date::operator=(const Date&)' is deprecated [-Wdeprecated-copy]
  120 |         date = Date::parse(text);
      |                                ^
In file included from include/Allocation.h:5,
                 from include/DistributionLog.h:4,
                 from include/CsvStore.h:8,
                 from src/CsvStore.cpp:1:
include/Date.h:16:5: note: because 'Date' has user-provided 'Date::Date(const Date&)'
   16 |     Date(const Date& other);               // copy constructor
      |     ^~~~
src/Stock.cpp: In member function 'void Stock::setDailyDemand(const Ration&)':
src/Stock.cpp:36:66: warning: implicitly-declared 'constexpr Ration& Ration::operator=(const Ration&)' is deprecated [-Wdeprecated-copy]
   36 | void Stock::setDailyDemand(const Ration& demand) { dailyDemand = demand; }
      |                                                                  ^~~~~~
In file included from include/Stock.h:5,
                 from src/Stock.cpp:1:
include/Ration.h:16:5: note: because 'Ration' has user-provided 'Ration::Ration(const Ration&)'
   16 |     Ration(const Ration& other);
      |     ^~~~~~
```

The suite on that first g++ run: `passed: 1923   failed: 0`.

**Diagnosis.** `Date` and `Ration` declare a copy constructor but no copy
assignment operator. Since C++11 the implicit assignment is deprecated in that
situation and GCC's `-Wextra` diagnoses it (`-Wdeprecated-copy`); Apple clang
does not. No bug: behaviour is unchanged.

**Fix (behaviour-neutral).** Declare the assignment explicitly as defaulted, on
the same source line as the copy constructor so no `[Tag]` text and no
line number in `docs/OOP_MAPPING.md` moved:

```text
include/Date.h   : Date(const Date& other); Date& operator=(const Date&) = default;
include/Ration.h : Ration(const Ration& other); Ration& operator=(const Ration&) = default;
```

### 5b. After the fix, g++-15 build

Command `make clean && make CXX=g++-15`: exit 0, **0 warnings**:

```text
rm -rf build bin
mkdir -p bin
g++-15 -std=c++17 -Wall -Wextra -pedantic -Iinclude <all src/*.cpp ...> -o ... -pthread   [command line shortened]
```

### 5c. After the fix, g++-15 test suite

Command `make clean && make test CXX=g++-15`: exit 0, 0 warnings, same suites as
section 2. Summary line and its context:

```text

=========================================
  passed: 1923   failed: 0
=========================================
```

Clang was re-run after the fix too: `make` and `make test` both 0 warnings,
`passed: 1923   failed: 0`.

## 6. `make seed` determinism

Method: md5 of the 9 files in `data/` before, `make seed`, md5 after, then `git status`.

```text
c++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src/Adult.c ...

Seeded data:
  camps      : 3
  families   : 40
  volunteers : 4

Demo-critical values:
  camp 2 free space    : 2   (demo step 3 needs <= 2)
  camp 2 rice days     : 2   (demo step 5 needs < 3)
  camp 3 rice days     : 18   (demo step 5 needs > 7)
  camp 2 diarrhoea/3d  : 7   (demo step 6 needs < 10)
--- git status --short data ; git diff --stat data ---
(no output)
--- md5 before vs after ---
IDENTICAL (all 9 files)
```

`data/` already equals the committed state, so no `git checkout -- data/` was needed.

## 7. `bash tests/demo_smoke.sh`

Note: the script exits with `FAIL: ./bin/relief not built` if `bin/relief` is
missing (for example right after `make test`, which rebuilds only `bin/tests`
after `make clean`). That is its designed precondition, not a defect; run
`make` first.

```text
$ make && bash tests/demo_smoke.sh
PASS: demo smoke
```

## 8. The eight-step demo

Setup: the real binary was driven by piped input, against a **copy** of
`data/` (here `/tmp/demo_data`), passed as `argv[1]`. The committed `data/` was
never touched (`git status data/` is empty afterwards). The app's date is fixed
at 2026-07-14.

Session 1 input (one value per line; family 41 is the new family):

```text
2 Test Family Mayong Baby 1 F n n n y Granny 70 F n n n y Mother 30 F n n n n n 3 41 2 3 41 1 4 1 4 1 5 2 7 1 5 2 8 2 1 2 8 2 1 3 10 2 y 0
```

Session 1 output below keeps the prompts as printed (the prompts run together
because stdin is piped, so there is no echoed newline). The repeated main menu
and the family list are trimmed where marked.

### Step 1: app starts and loads seeded data

```text
Loaded 3 camps and 40 families from /tmp/demo_data

==========================================
  RELIEF CAMP MANAGER
  Morigaon district
  Date: 2026-07-14
  offline - no internet required
==========================================
Autosave is running every 30 seconds.
```

### Step 2: register a family with an infant and an elderly member

Members: Baby (1, infant), Granny (70, elderly), Mother (30, adult). Computed
ration and vulnerability score (85, at or above the 75 floor from the infant and the elderly member alone):

```text
Choose: Head of family (blank = cancel): Village: Member name (blank = finish): Age: Sex (M/F/O): Pregnant? (y/n): Disabled? (y/n): Breastfeeding? (y/n): Add another member? (y/n): Member name (blank = finish): Age: Sex (M/F/O): Pregnant? (y/n): Disabled? (y/n): Breastfeeding? (y/n): Add another member? (y/n): Member name (blank = finish): Age: Sex (M/F/O): Pregnant? (y/n): Disabled? (y/n): Breastfeeding? (y/n): Will they take a camp duty? (y/n): Add another member? (y/n): 
+------------------------------------------+
| RATION CARD
+------------------------------------------+
| Family ID     : 41
| Head          : Test Family
| Camp          : not yet admitted
| Members       : 3 (infants 1, children 0, adults 1, elderly 1)
| Daily ration  :
|   Rice 0.7 kg
|   Dal 0.12 kg
|   Oil 0.075 litre
|   Salt 0.022 kg
|   ORS 0.55 packets
|   Baby food 0.1 kg
| Vulnerability : 85
+------------------------------------------+

Family id 41, vulnerability score 85
```

### Step 3: admit to a full camp: exception caught, alternatives suggested

```text
Admit which family (family id, 0 = cancel):   Camp #1 Bhuragaon HS School, Morigaon [48/180]
  Camp #2 Laharighat Community Hall, Morigaon [88/90]
  Camp #3 Mayong Embankment Tents, Morigaon [29/120]
Admit to which camp (camp id, 0 = cancel): 
  REFUSED: camp 2 cannot take 3 more people; only 2 space(s) free
  Camps with room for 3 people:
    Camp #1 Bhuragaon HS School, Morigaon [48/180]  (free: 132)
    Camp #3 Mayong Embankment Tents, Morigaon [29/120]  (free: 91)
```

Then the same family admitted to camp 1:

```text
Admitted family 41 (3 people) to Bhuragaon HS School, shelter Room-1. Camp now 51/180.
```

### Step 4: distribute, then the same claim the same day is blocked

First round at camp 1 (first 6 of 17 lines shown, served in order of vulnerability):

```text
  Served family 9 Anima Borah (vulnerability 130)
  Served family 1 Bipul Das (vulnerability 125)
  Served family 13 Junu Sarma (vulnerability 105)
  Served family 7 Manju Deka (vulnerability 90)
  Served family 4 Hemanta Bora (vulnerability 85)
  Served family 41 Test Family (vulnerability 85)
  ... [11 more "Served family" lines trimmed] ...
Round finished at Bhuragaon HS School: 17 served, 0 blocked, 0 short of stock.
```

Second round, same day (3 of 17 lines shown):

```text
  BLOCKED: family 9 has already collected rations on 2026-07-14
  BLOCKED: family 1 has already collected rations on 2026-07-14
  BLOCKED: family 13 has already collected rations on 2026-07-14
  ... [14 more BLOCKED lines trimmed] ...
Round finished at Bhuragaon HS School: 0 served, 17 blocked, 0 short of stock.
```

### Step 5: stock, days of supply, critical item, transfer

Camp 2 before (rice is critical and flagged LOW), then the transfer suggestion
(option 7) and applying it, then camp 2 after:

```text
View stock of which camp (camp id, 0 = cancel):   Rice          44.4 kg          2.0 days  LOW
  Dal           31.4 kg          8.0 days
  Oil           16.2 litre       8.0 days
  Salt           5.9 kg          8.0 days
  ORS          100.8 packets     8.0 days
  Baby food      3.2 kg          8.0 days
  Tarpaulin     40.0 sheets  no daily demand
Worst item: Rice (2.0 days)
Days of supply: 2.0 (critical item: Rice)  LOW

Choose: 1. Move 22.2 kg of Rice from Mayong Embankment Tents (#3, 18.0 days) to Laharighat Community Hall (#2, 2.0 days)
Apply which suggestion (0 = none): Moved 22.2 kg of Rice from Mayong Embankment Tents to Laharighat Community Hall.


View stock of which camp (camp id, 0 = cancel):   Rice          66.6 kg          3.0 days
  Dal           31.4 kg          8.0 days
  Oil           16.2 litre       8.0 days
  Salt           5.9 kg          8.0 days
  ORS          100.8 packets     8.0 days
  Baby food      3.2 kg          8.0 days
  Tarpaulin     40.0 sheets  no daily demand
Worst item: Rice (3.0 days)
Days of supply: 3.0 (critical item: Rice)
```

Rice rose from 2.0 to 3.0 days and the LOW flag is gone (3.0 is not "above" 3 in a strict sense, see notes at the top).

### Step 6: log health cases until the outbreak alert fires

Diarrhoea at camp 2 started at 7 cases in the 3-day window. Logged 2 (total 9, no alert), then 3 (total 12, alert):

```text
Symptom number (0 = cancel): Number of cases: Logged 2 case(s) of Diarrhoea at Laharighat Community Hall. Last 3 days total: 9
Symptom number (0 = cancel): Number of cases: Logged 3 case(s) of Diarrhoea at Laharighat Community Hall. Last 3 days total: 12

  ********************************
  *** OUTBREAK ALERT ***
  Diarrhoea at Laharighat Community Hall: 12 cases in 3 days (threshold 10)
  ********************************
```

### Step 7: daily report generated and exported

Report for camp 2 (option 10). Only the FAMILIES table is trimmed:

```text
=====================================================
 DAILY CAMP REPORT   2026-07-14
=====================================================
 Camp      : #2  Laharighat Community Hall
 District  : Morigaon
 Occupancy : 88 / 90

SHELTERS
  Hall-B: 59 / 60
  Tent-1: 29 / 30

STOCK
  Rice          66.6 kg          3.0 days
  Dal           31.4 kg          8.0 days
  Oil           16.2 litre       8.0 days
  Salt           5.9 kg          8.0 days
  ORS          100.8 packets     8.0 days
  Baby food      3.2 kg          8.0 days
  Tarpaulin     40.0 sheets  no daily demand
Worst item: Rice (3.0 days)

DISTRIBUTION
Served today: 0 of 14 families
  Nothing issued today

HEALTH
  Diarrhoea: 12 cases in the last 3 days
  *** OUTBREAK ALERT: Diarrhoea ***

FAMILIES
  #17  Dipti Rajbongshi  (7 people, vulnerability 145)  PENDING
  #18  Tarun Saikia  (6 people, vulnerability 100)  PENDING
  #19  Lakhi Nath  (8 people, vulnerability 185)  PENDING
  #20  Mrinal Baruah  (6 people, vulnerability 100)  PENDING
  #21  Bina Kalita  (8 people, vulnerability 165)  PENDING
  #22  Haren Bora  (7 people, vulnerability 155)  PENDING
  #23  Sewali Deka  (6 people, vulnerability 100)  PENDING
  #24  Utpal Pegu  (5 people, vulnerability 80)  PENDING
  #25  Karuna Medhi  (7 people, vulnerability 165)  PENDING
  #26  Gopal Hazarika  (5 people, vulnerability 80)  PENDING
  ... [FAMILIES table: 4 more PENDING rows trimmed] ...
Export this report to CSV? (y/n): Exported to /tmp/demo_data/report_2026-07-14_camp2.csv
```

The export line and the CSV file written (first 20 lines of `report_2026-07-14_camp2.csv`):

```text
Exported to /tmp/demo_data/report_2026-07-14_camp2.csv
--- head -20 of the file ---
section,key,value
report,date,2026-07-14
camp,id,2
camp,name,Laharighat Community Hall
camp,district,Morigaon
camp,occupancy,88
camp,capacity,90
shelter,Hall-B,59/60
shelter,Tent-1,29/30
stock,Rice,66.6
stock,Dal,31.36
stock,Oil,16.24
stock,Salt,5.888
stock,ORS,100.8
stock,Baby food,3.2
stock,Tarpaulin,40
distribution,families_served,0
health,Diarrhoea,12
family,Dipti Rajbongshi,pending
family,Tarun Saikia,pending
... [rest: 12 more family,<name>,pending rows] ...
```

### Step 8: quit with autosave, restart, data intact

Quit (option 0) printed:

```text
Stopping autosave and writing final save...
Saved. 0 autosave(s) ran this session.
```

The saved files contain the new family (rows for family 41 and its three members, and its distribution):

```text
families.csv : 41,Test Family,Mayong,1
members.csv  : 335,41,INFANT,Baby,1,F,0,0,0,,,Room-1
               336,41,ELDERLY,Granny,70,F,0,0,0,,,Room-1
               337,41,ADULT,Mother,30,F,0,0,0,,,Room-1
distributions.csv : 41,1,2026-07-14,0.7,0.12,0.075,0.022,0.55,0.1,0
```

Restart: a second process against the **same copy**, input `1 4 1 5 2 10 2 n 0`
(list camps, distribute at camp 1, view stock at camp 2, report for camp 2, decline export, quit). Prompts and the menu are trimmed as before:

```text
Loaded 3 camps and 41 families from /tmp/demo_data

==========================================
  RELIEF CAMP MANAGER
  Morigaon district
  Date: 2026-07-14
  offline - no internet required
==========================================
Autosave is running every 30 seconds.

Choose: Camp #1 Bhuragaon HS School, Morigaon [51/180]
    shelters: 3, days of supply: 9.5
Camp #2 Laharighat Community Hall, Morigaon [88/90]
    shelters: 2, days of supply: 3.0
Camp #3 Mayong Embankment Tents, Morigaon [29/120]
    shelters: 3, days of supply: 9.0

Choose:   Camp #1 Bhuragaon HS School, Morigaon [51/180]
  Camp #2 Laharighat Community Hall, Morigaon [88/90]
  Camp #3 Mayong Embankment Tents, Morigaon [29/120]
  BLOCKED: family 9 has already collected rations on 2026-07-14
  BLOCKED: family 1 has already collected rations on 2026-07-14
  BLOCKED: family 13 has already collected rations on 2026-07-14
  BLOCKED: family 7 has already collected rations on 2026-07-14
  BLOCKED: family 4 has already collected rations on 2026-07-14
  BLOCKED: family 41 has already collected rations on 2026-07-14
  BLOCKED: family 11 has already collected rations on 2026-07-14
  BLOCKED: family 16 has already collected rations on 2026-07-14
  BLOCKED: family 5 has already collected rations on 2026-07-14
  BLOCKED: family 10 has already collected rations on 2026-07-14
  BLOCKED: family 15 has already collected rations on 2026-07-14
  BLOCKED: family 3 has already collected rations on 2026-07-14
  BLOCKED: family 8 has already collected rations on 2026-07-14
  BLOCKED: family 6 has already collected rations on 2026-07-14
  BLOCKED: family 12 has already collected rations on 2026-07-14
  BLOCKED: family 14 has already collected rations on 2026-07-14
  BLOCKED: family 2 has already collected rations on 2026-07-14
Round finished at Bhuragaon HS School: 0 served, 17 blocked, 0 short of stock.

Choose:   Camp #1 Bhuragaon HS School, Morigaon [51/180]
  Camp #2 Laharighat Community Hall, Morigaon [88/90]
  Camp #3 Mayong Embankment Tents, Morigaon [29/120]
View stock of which camp (camp id, 0 = cancel):   Rice          66.6 kg          3.0 days
  Dal           31.4 kg          8.0 days
  Oil           16.2 litre       8.0 days
  Salt           5.9 kg          8.0 days
  ORS          100.8 packets     8.0 days
  Baby food      3.2 kg          8.0 days
  Tarpaulin     40.0 sheets  no daily demand
Worst item: Rice (3.0 days)
Days of supply: 3.0 (critical item: Rice)

Choose:   Camp #1 Bhuragaon HS School, Morigaon [51/180]
  Camp #2 Laharighat Community Hall, Morigaon [88/90]
  Camp #3 Mayong Embankment Tents, Morigaon [29/120]
Report for which camp (camp id, 0 = cancel): =====================================================
 DAILY CAMP REPORT   2026-07-14
=====================================================
 Camp      : #2  Laharighat Community Hall
 District  : Morigaon
 Occupancy : 88 / 90

SHELTERS
  Hall-B: 59 / 60
  Tent-1: 29 / 30

STOCK
  Rice          66.6 kg          3.0 days
  Dal           31.4 kg          8.0 days
  Oil           16.2 litre       8.0 days
  Salt           5.9 kg          8.0 days
  ORS          100.8 packets     8.0 days
  Baby food      3.2 kg          8.0 days
  Tarpaulin     40.0 sheets  no daily demand
Worst item: Rice (3.0 days)

DISTRIBUTION
Served today: 0 of 14 families
  Nothing issued today

HEALTH
  Diarrhoea: 12 cases in the last 3 days
  *** OUTBREAK ALERT: Diarrhoea ***

FAMILIES
  #17  Dipti Rajbongshi  (7 people, vulnerability 145)  PENDING
  #18  Tarun Saikia  (6 people, vulnerability 100)  PENDING
  #19  Lakhi Nath  (8 people, vulnerability 185)  PENDING
  ... [11 more PENDING rows trimmed] ...
Export this report to CSV? (y/n): 
Choose: 
Stopping autosave and writing final save...
Saved. 0 autosave(s) ran this session.
```

What this shows after the restart:

* `Loaded 3 camps and 41 families`: was 40, so the registered family persisted.
* Camp 1 occupancy is `51/180`, the admission persisted.
* Distribution at camp 1: all 17 families (including 41) are `BLOCKED ... already collected rations on 2026-07-14`, so the claim log persisted.
* Camp 2 rice is `66.6 kg, 3.0 days`, the transfer persisted.
* The camp 2 report shows `Diarrhoea: 12 cases` and `OUTBREAK ALERT`, so the health log persisted.

## Syllabus-mapping check

`sh tools/check_mapping.sh` after all edits: `PASS: 111 tags in source match the document index; citations in range; every concept tagged`.
