# Design: Offline Relief Camp Manager (Assam Floods)

Date: 2026-11-11
Status: approved
Course: Object Oriented Programming (CS 203), DTU, 3rd semester
Also: Seva First Innovation Challenge 2026, Theme 2, REG-009-OP

## 1. Purpose and constraints

An offline-first, console-based relief camp management system in C++17 that runs
on one low-cost laptop with no internet, no database server and no third-party
libraries. Data persists as plain CSV so it survives a power cut.

Hard constraints, carried over from `PROJECT_CONTEXT.md`:

- C++17, standard library only. Builds with `g++` on Linux, macOS and Windows
  (MinGW). Both a `Makefile` and a `CMakeLists.txt` are provided.
- Console menu UI, clear and demo-friendly. No GUI, no networking, no auth.
- Readable for a 3rd-semester student. No template metaprogramming. STL
  containers internally are fine, but every syllabus construct is written by us
  and clearly visible, each tagged with a comment naming the concept, e.g.
  `// [Virtual base class]`.
- One class per header/source pair. Folders: `include/`, `src/`, `tests/`,
  `data/`.
- Tests use a self-written assert-based runner in `tests/`. Nothing downloaded.

## 2. Decisions taken during design

Four design questions were settled explicitly:

1. **Person hierarchy shape: one tree.** The age types hang off `Resident`, and
   `Person` is the abstract virtual base of both `Resident` and `Volunteer`.
   Rejected: two separate trees (the word "resident" would mean two different
   things in the codebase) and roles layered on `Adult` (the virtual base would
   then demonstrate on `Adult`, not `Person`).
2. **Rule of three lives on `Family`,** which owns a raw `Person**` dynamic
   array and deep-copies through a virtual `clone()`. `Registry<T>` uses
   `std::unique_ptr<T>` so both ownership styles are visible side by side.
   Rejected: `unique_ptr` in `Family` with the rule of three demonstrated on
   `Stock` (the copy constructor would look decorative there).
3. **Small focused collaborators behind a thin `ConsoleApp`.** Entities guard
   their own invariants; each cross-entity concern is its own small class.
   Rejected: a single large `CampManager` facade, and putting all logic on the
   entities (the duplicate-claim check spans all camps, so it has no single
   camp to live in).
4. **Hand-written priority queue.** A binary heap we write ourselves, ordered by
   our overloaded `Family::operator<`. Rejected: `std::priority_queue`, which
   would hide the data structure the syllabus row is pointing at.

## 3. The Person hierarchy

```
                          +------------------------------+
                          |          Person              | <<abstract>>
                          |  static int nextId           |  static data member
                          |  id, name, age, sex,         |
                          |  pregnant, disabled,         |
                          |  lactating                   |
                          +------------------------------+
                          | Person()                     | default
                          | Person(name, age, sex)       | parameterised
                          | Person(const Person&)        | copy
                          | virtual ~Person()            | prints in debug mode
                          | static int generateId()      |
                          | = rationNeed() : Ration      | pure virtual
                          | = vulnerabilityScore() : int | pure virtual
                          | = describe() : string        | pure virtual
                          | = clone() : Person*          | pure virtual
                          | friend operator<<            |
                          +--------------+---------------+
                 virtual public          |        virtual public
            +--------------------------- + ---------------------------+
            |                                                        |
+-----------v------------+                           +---------------v------------+
|       Resident         | <<abstract>>              |        Volunteer           |
|  familyId, campId,     |                           |  skill, shift,             |
|  shelterLabel          |                           |  organisation              |
|  (no ration rule yet)  |                           |  rationNeed = worker meal  |
+-----------+------------+                           |  vulnerability = low       |
    +-------+----------------+                       +---------------+------------+
    |       |                |                                       |
+---v----+ +v--------+ +-----v------+                                |
| Infant | |  Child  | |   Adult    |                                |
| 0-2 y  | | 3-12 y  | |  13-59 y   |                                |
| baby   | | half    | | full norm  |                                |
| food,  | | rice/   | | +pregnant  |                                |
| ORS,   | | dal     | | +lactating |                                |
| no rice| |         | |            |                                |
+--------+ +---------+ +-----+------+                                |
                             |                                       |
                   +---------+----------+                            |
                   |                    |                            |
          +--------v--------+  +--------v----------------------------v--------+
          |    Elderly      |  |          ResidentVolunteer                   |
          |     60+ y       |  |  a displaced adult who takes camp duty       |
          |  less rice,     |  |  rationNeed()  -> Adult::rationNeed()        |
          |  more ORS/oil   |  |  vulnScore()   -> Adult::vulnerabilityScore()|
          +-----------------+  |  describe()    -> "resident volunteer"       |
                               +----------------------------------------------+
```

Inheritance kinds:

| Kind | Where |
|---|---|
| Single | `Infant : public Resident` |
| Multilevel | `Elderly -> Adult -> Resident -> Person` (four levels) |
| Hierarchical | `Infant`, `Child`, `Adult` all derive from `Resident` |
| Multiple | `ResidentVolunteer : public Adult, public Volunteer` |
| Hybrid | the whole figure: hierarchical + multiple + virtual |
| Virtual base | `Resident : virtual public Person`, `Volunteer : virtual public Person` |

The `virtual` keyword earns its place. Without it a `ResidentVolunteer` would
carry two `Person` subobjects, so it would have two IDs and two sets of
vulnerability flags, and `describe()` would be ambiguous. With it there is
exactly one `Person`, and `ResidentVolunteer`'s constructor initialises that
base directly: the intermediate constructors' `Person(...)` calls are ignored
for the most-derived object. This rule is commented at the line where it
applies.

## 4. Value types and conversions

```
Item        enum class { Rice, Dal, Oil, Salt, ORS, BabyFood, Tarpaulin }
Date        y/m/d; operator==, operator<, operator- (days between), operator<<
Symptom     enum class { Diarrhoea, Fever, SkinInfection, Respiratory, Other }

RationNorms   static const tables + static functions (forAdult, forInfant, ...)
Ration        qty[ITEM_COUNT]; a daily entitlement
              Ration(int persons)                  <- basic to class
              operator+  operator-  operator* (scale)
              friend operator<<
Stock         qty[ITEM_COUNT]; what a camp physically holds
              friend operator+(Stock, Stock)       <- friend binary
              friend operator-(Stock, Allocation)  <- objects as arguments
              operator+=  operator-=  operator[](Item)
              operator double()                    <- class to basic: days of supply
Allocation    familyId, date, Ration amount (one handout)
              operator Ration() const              <- class to class
RationCard    printable entitlement card built from a Family
              Family::operator RationCard() const  <- class to class
```

`Stock::operator double()` returns the worst-case days of supply across items:
the minimum of `qty[i] / demand[i]` over every item whose daily demand is
greater than zero, using a daily-demand `Ration` the camp sets on its stock. If
no item has recorded demand it returns `Stock::kUnlimitedDays` (999.0), meaning
"no draw recorded". `operator-(Stock, Allocation)` returns a new `Stock` and
throws `InsufficientStockException` if any item would go negative. This
conversion is the one that is engineered rather than natural; see section 7.

## 5. Entities and collaborators

```
Family                        Camp
  static nextId                 static nextId
  Person** members  <-----+     id, name, district, capacity, occupancy
  int count, capacity     |     vector<Shelter> shelters
  Family(const Family&)   |     Stock stock
  operator=               +- rule of three
  ~Family()               |     class Shelter        <- nested class
  addMember(Person*)      |       label, capacity, occupancy
  dailyRation() : Ration  |     admit(const Family&) throws OverCapacityException
  vulnerabilityScore()    |     operator++ / operator-- (prefix and postfix)
  operator RationCard()   |     isOverflowing(), occupancyRate() const
  friend operator< == <<  |     friend class ReportGenerator
  double-returning metric |     friend operator<<
  functions for pointer-to-member sorting
```

```
Registry<T>          template: add / findById / remove / at / all / size
                     holds vector<unique_ptr<T>>, a deliberate contrast to Family
findMax<T>           function template taking a pointer-to-member metric

Exceptions.h         ReliefException : std::exception
                       +- OverCapacityException      (campId, requested, free)
                       +- InsufficientStockException (item, needed, available)
                       +- DuplicateClaimException    (familyId, date)
                       +- FileFormatException        (file, line number)

DistributionLog      record(...) throws DuplicateClaimException when (familyId,
                     date) already exists in ANY camp; uses Distribution::operator==
StockLedger          stockIn / stockOut per camp per item, with history
TransferAdvisor      suggest(camps, demand) -> vector<TransferSuggestion>
HealthMonitor        logCase(...); outbreakAlert(campId, today, symptom) sums the
                     window [today-2, today] for one camp and compares it with
                     static thresholds: 10 diarrhoea cases or 15 fever cases in
                     3 days (static const members, so the norms sit in one place)
ReportGenerator      friend of Camp; dailyReport(const Camp&) + exportCsv(...)
CsvStore             save/load every registry and log; FileFormatException on bad rows
Autosaver            std::thread + std::condition_variable + atomic<bool>, sharing
                     the app's std::mutex; clean join on stop(). Default interval
                     30 seconds, passed in by ConsoleApp so tests can use 1
DistributionQueue    hand-written binary heap over Family*, ordered by operator<
FamilySorter         typedef double (Family::*Metric)() const;  <- pointer to member
ConsoleApp           owns the registries and collaborators; drives the menu
```

## 6. Error handling, data flow, testing

**Error handling.** Entities throw, the menu catches. `Camp::admit` throws
`OverCapacityException` carrying the camp ID and remaining space, so the catch
block in `ConsoleApp` can suggest another camp with room. Nothing below
`ConsoleApp` prints an error message.

**Data flow for one distribution.** `ConsoleApp` locks the shared mutex, builds
a `DistributionQueue` of the camp's families, pops the most vulnerable, calls
`family.dailyRation()` which walks the `Person*` members through the pure
virtual `rationNeed()`, asks `DistributionLog::record` to reject a repeat claim,
debits the camp with `Stock - Allocation`, and unlocks. The autosave thread
blocks on the same mutex, so it can never write a half-updated state.

**Testing.** `tests/TestRunner.h` is header-only and provides `ASSERT_TRUE`,
`ASSERT_EQ`, `ASSERT_NEAR`, `ASSERT_THROWS`, a pass/fail tally and a non-zero
exit code on failure. Each area gets one file exposing a single `run_*_tests()`
function that `tests/test_main.cpp` calls, so there is no registration magic.
`make test` builds and runs the suite. Every task in the implementation plan
writes its failing test first.

## 7. Syllabus coverage: honest assessment

Fourteen of the seventeen rows fall out of the domain on their own. Three are
pedagogical choices, named here rather than glossed over.

| Row | Verdict |
|---|---|
| Classes, objects, constructors (default/parameterised/copy), destructors | Natural. Every entity needs all three constructor forms; `Family`'s destructor does real work. |
| Constant objects and const member functions | Natural. Every getter is `const`; `ReportGenerator` takes `const Camp&`. |
| Static data members and static member functions | Natural. ID generators, and `RationNorms` is genuinely one shared table. |
| Friend function and friend class | Natural. `ReportGenerator` needs `Camp`'s private occupancy and stock; `operator<<` must be a friend to be a free function. |
| Objects as arguments and returning objects | Natural. `admit(const Family&)`, `dailyRation() -> Ration`, `suggest() -> TransferSuggestion`. |
| Nested class | Natural. A shelter has no meaning outside its camp. |
| Dynamic memory and dynamic objects, arrays of objects | **Pedagogical.** `std::vector<std::unique_ptr<Person>>` is the real answer. Raw `Person**` is used in `Family` because the syllabus asks for it, with `unique_ptr` in `Registry<T>` so the two sit side by side and `OOP_MAPPING.md` can point at both. |
| Pointers to members | Natural enough. The menu offers "sort families by vulnerability / size / average age / infants", and one `Metric` pointer replaces four near-identical sort functions. |
| Operator overloading: unary, binary, friend-based | Mostly natural. `+`, `-`, `<`, `==`, `<<` all do real work. `++camp` / `--camp` for occupancy is sugar over `admit`/`release`; it reads fine but it is the one cute operator. |
| Type conversion: class to basic, basic to class, class to class | Two natural, one engineered. `Family -> RationCard` and `int -> Ration` are things an official would want. `Stock -> double` as days-of-supply requires `Stock` to remember the camp's current daily draw, which is contrived; a `daysOfSupply(demand)` method would be cleaner. Kept as specified, with a comment explaining the trade-off. |
| Inheritance: single, multilevel, hierarchical, multiple, hybrid | Natural. See section 3. |
| Virtual base class | Natural. A resident who volunteers is common in Assam camps, and the duplicate-`Person` bug is real. |
| Abstract class, pure virtual functions, runtime polymorphism | Very natural. This is the engine of the ration calculation. |
| Templates (class and function) | Natural. Camps, families and volunteers need the same store / find / remove. |
| Exception handling with custom exception classes | Very natural. Four of the eight demo steps are exception paths. |
| File handling | Very natural. Offline-first CSV is the premise of the project. |
| Multithreading | Natural. Autosaving against a power cut is the stated motivation, not a bolt-on. |

`docs/OOP_MAPPING.md` will list every row above with file and line references so
the professor can verify coverage quickly.

## 8. Seed data

Morigaon district, three camps: a higher-secondary school, a community hall and
an embankment tent cluster. About 40 families with realistic Assamese names and
a mix of infants, pregnant women, disabled and elderly members. Stock levels are
seeded so one item is already critical and one camp holds a surplus of it, so
the transfer suggestion and the outbreak alert fire on the seeded data without
any setup typing during the demo.

## 9. Demo script the build must satisfy

1. Start the app; it loads seeded data from `data/`.
2. Register a new family with an infant and an elderly member; show the computed
   ration and vulnerability score.
3. Try to admit them to a full camp; the overflow exception is caught and the app
   suggests another camp.
4. Distribute rations, then try again the same day; the duplicate claim is blocked.
5. Show stock and days-of-supply; one item is critical, so the app suggests a
   transfer from another camp.
6. Log health cases, triggering an outbreak alert.
7. Generate and export the daily report.
8. Quit; autosave runs, and on restart the data is intact.

## 10. Out of scope

GUI, networking, login and authentication, multi-language UI, SMS.
