# OOP Syllabus Coverage Map

Object Oriented Programming (CS 203) - Offline Relief Camp Manager

Every row of the course syllabus, with the file and line where it is
demonstrated. The source carries a bracketed tag comment (for example
`// [Friend class]`) on or next to each construct, so
`grep -rn -i "\[friend class" include src` finds any row yourself. Line numbers
were checked against the repository at the commit that added this file.
The mapping is reproducible: `sh tools/collect_tags.sh` lists every tag with its
file and line, and `sh tools/check_mapping.sh` fails if the tag index at the end
of this file no longer matches the source (run both from the repository root).

## Honest assessment: which rows are natural, which are taught

Most rows fall out of the relief-camp domain on their own: an infant, a child
and an adult really do ration differently (inheritance, virtual functions), a
resident who also volunteers really is common in Assam camps (the diamond), and
offline CSV storage and an autosave thread are the premise of the project.

Three constructs exist **because the syllabus asks for them**, and a production
design would do otherwise. They are deliberate teaching choices:

1. **`Family` owns a raw `Person**` array** with a hand-written rule of three
   (`include/Family.h:17-49`, `src/Family.cpp:23-90`). The real answer is
   `std::vector<std::unique_ptr<Person>>`. The project shows both on purpose:
   `Registry<T>` (`include/Registry.h:18`) is the `unique_ptr` counterpart, and
   the contrast between the two is the teaching point.
2. **`++camp` / `--camp`** (`include/Camp.h:74-78`, `src/Camp.cpp:106-127`) are
   sugar over `admit`/`release`. They read fine but are the one cute operator.
3. **`Stock::operator double()` as days of supply** (`include/Stock.h:36-40`,
   `src/Stock.cpp:67`). It forces `Stock` to remember the camp's daily demand
   (`include/Stock.h:15`) so a no-argument conversion can work, where a
   `daysOfSupply(demand)` method would be cleaner. The header comment says so.

Everything else is natural to the domain. "Mostly natural" also applies to
pointer-to-member (four sort metrics collapse into one routine) and to the
other operators (`+`, `-`, `<`, `==`, `<<` all do real work).

## Coverage table (17 syllabus rows)

| # | Syllabus row | Where | What to look at |
|---|---|---|---|
| 1 | Classes, objects, constructors (default, parameterised, copy), destructors | `include/Person.h:28-31`; `include/Family.h:41-49`; `src/Family.cpp:23-56`; `include/Ration.h:15`; `include/Debug.h:7`; `src/Person.cpp:33` | `Person` and `Family` show all three constructor forms. `Family`'s destructor frees its members. `make debug` prints `[lifecycle] ... created / copied / destroyed` lines (`src/Family.cpp:53`, `src/Person.cpp:33`). `Person`'s destructor is virtual (`include/Person.h:31`). |
| 2 | Constant objects and const member functions | `include/Person.h:39`; `include/Date.h:19`; `include/Ration.h:19`; `include/Registry.h:36`; `include/ReportGenerator.h:21` | Every getter is `const`. Every `ReportGenerator` parameter is a `const` reference, so a report cannot change a camp. |
| 3 | Static data members and static member functions | `include/Person.h:11,33`; `src/Person.cpp:4`; `include/Camp.h:37,57`; `include/Family.h:24,52`; `include/Stock.h:17`; `include/RationNorms.h:11,20`; `include/HealthMonitor.h:34,52`; `include/Csv.h:12` | Auto-increment ID generators (`Camp::generateId`, `src/Camp.cpp:75`) and the ration-norm table, which is genuinely one shared table. Definitions of the statics sit in one `.cpp` each. |
| 4 | Friend function and friend class | Class: `include/Camp.h:93`, used at `src/ReportGenerator.cpp:50,100`. Functions: `include/Person.h:59`; `include/Allocation.h:30`; `include/Date.h:33`; `include/Family.h:88`; `include/Ration.h:31` | `friend class ReportGenerator;` lets the report read `Camp`'s private shelters, occupancy and stock. `operator<<` and the binary operators are friend free functions. |
| 5 | Objects as arguments and returning objects | Arguments: `include/Camp.h:70`; `include/Stock.h:44`; `include/TransferAdvisor.h:60`; `src/TransferAdvisor.cpp:78`. Returning: `include/Date.h:26`; `include/Family.h:72`; `src/Adult.cpp:29`; `include/TransferAdvisor.h:60` | `Camp::admit(const Family&)`, `Family::dailyRation() -> Ration`, `Date::plusDays`, the transfer suggestion returned by value. |
| 6 | Nested class | `include/Camp.h:16-18` | `Camp::Shelter`. A shelter has no meaning outside the camp that contains it. |
| 7 | Dynamic memory and dynamic objects, arrays of objects | `include/Family.h:17-18`; `src/Family.cpp:59,62,85,86`; `src/Family.cpp:65` | `Family` holds `Person**`, grown with `new Person*[]` and filled with `clone()`d members, with a hand-written copy constructor (`src/Family.cpp:26`), copy assignment (`:35`) and destructor (`:51`). Smart-pointer counterpart: `include/Registry.h:18`. **Pedagogical, see above.** |
| 8 | Pointers to members | `include/Algorithms.h:6,12-13`; `include/FamilySorter.h:11,20`; `include/Family.h:76-80`; `src/FamilySorter.cpp:13,30`; `src/ConsoleApp.cpp:592` | A `Metric` is `double (Family::*)() const`, called as `(f->*metric)()`. One sort routine covers vulnerability, size, average age and infant count. `findMax` is the function-template form. |
| 9 | Operator overloading: unary, binary, friend-based | Unary: `include/Camp.h:74-78`. Binary: `include/Ration.h:26-29`; `include/Stock.h:42-44`; `include/Date.h:38`. Friend: `include/Stock.h:46-49`; `include/Ration.h:31-36`; `include/Family.h:88-91`; `include/Allocation.h:30-34`; `include/Date.h:33-39`; `src/DistributionQueue.cpp:6` | `Stock + Stock`, `Stock - Allocation`, `Family <` for priority (used by the heap, `src/DistributionQueue.cpp:6`), `==` for the duplicate-claim check (`include/Allocation.h:30`), `<<` for printing, `Date - Date` for days between. `++`/`--` on `Camp` is **pedagogical, see above.** |
| 10 | Type conversion: class to basic, basic to class, class to class | Class to basic: `include/Stock.h:36`, `src/Stock.cpp:67`. Basic to class: `include/Ration.h:12-13`. Class to class: `include/Family.h:83`, `src/Family.cpp:189`; `include/Allocation.h:26`, `src/Allocation.cpp:19` | `int -> Ration` is non-explicit on purpose. `Family -> RationCard` is the card an official hands over. `Allocation -> Ration` is what makes `stock - allocation` compile. `Stock -> double` is **pedagogical, see above.** |
| 11 | Inheritance: single, multilevel, hierarchical, multiple, hybrid | Single: `include/Adult.h:6`, `include/Child.h:6`, `include/Infant.h:6`. Hierarchical: `include/Adult.h:8`, `include/Child.h:8`, `include/Infant.h:8`. Multilevel: `include/Elderly.h:8`. Multiple: `include/ResidentVolunteer.h:10`. Hybrid: `include/ResidentVolunteer.h:12` | `Person -> Resident -> {Infant, Child, Adult}`; `Adult -> Elderly` (four levels); `ResidentVolunteer : Adult, Volunteer`. The tree as a whole is hybrid. |
| 12 | Virtual base class | `include/Resident.h:10,14`; `include/Volunteer.h:10,12`; `src/ResidentVolunteer.cpp:5,18`; `src/Adult.cpp:10`; `src/Child.cpp:10`; `src/Infant.cpp:10`; `src/Resident.cpp:9`; `src/Volunteer.cpp:10`; `src/ResidentVolunteer.cpp:35` | `class Resident : virtual public Person` and `class Volunteer : virtual public Person` give a `ResidentVolunteer` one `Person`, not two. The most-derived constructor initialises the virtual base. `[Ambiguity resolution]` (`src/ResidentVolunteer.cpp:35`) picks `Adult::rationNeed` where both bases supply one. Compiler evidence below. |
| 13 | Abstract class, pure virtual functions, runtime polymorphism | Abstract: `include/Person.h:8`. Pure virtual: `include/Person.h:52-57`. Overrides: `include/Adult.h:17`, `include/Child.h:16`, `include/Infant.h:16`; `include/Volunteer.h:29`. Dispatch: `include/Family.h:70`, `src/Family.cpp:148`; `include/CsvStore.h:94`, `src/CsvStore.cpp:196` | `Family::dailyRation` walks `Person*` and each member answers with its own `rationNeed()`. `CsvStore::makePerson` turns a CSV type tag back into the right concrete subclass, the one place that knows every subclass. |
| 14 | Templates (class and function) | Class: `include/Registry.h:7,16-17`. Function: `include/Algorithms.h:6,12-13`, used at `src/ConsoleApp.cpp:592` | `Registry<T>` stores, finds by ID and removes (camps, families, volunteers) and owns its elements through `unique_ptr`. `findMax` takes a member-function pointer. |
| 15 | Exception handling with custom exception classes | Base: `include/Exceptions.h:9,12`. Derived: `include/Exceptions.h:20,33,45,56` (classes at `:22,34,47,58`). Handlers: `src/ConsoleApp.cpp:692` (main), `src/ConsoleApp.cpp:428` (admit) | `ReliefException` is the base of `OverCapacityException`, `InsufficientStockException`, `DuplicateClaimException` and `FileFormatException`. The main loop catches the precise types first (`src/ConsoleApp.cpp:695-713`). Two `[Exception handling]` tags exist: the main loop, and `handleAdmit`, which catches the full-camp case locally so it can offer another camp. |
| 16 | File handling | `include/CsvStore.h:43`; `src/CsvStore.cpp:52-92,391,726`; `include/Csv.h:12` | One CSV per record kind, nine files. `saveAll` writes `<name>.tmp` for every file and renames them into place only when all were written, so a failed save leaves the old files intact. `loadAll` throws `FileFormatException` and leaves containers empty rather than half-loaded. Report export lives in `ReportGenerator`. |
| 17 | Multithreading | `include/Autosaver.h:27` (thread), `:29` (mutex), `:32` (atomic), `:38` (condition variable); `src/Autosaver.cpp:35-36,49,72-73`; `src/ConsoleApp.cpp:655` | A background `std::thread` saves every N seconds under the shared data mutex. The condition variable lets `stop()` wake it at once, then `join()` gives a clean shutdown. The destructor stops and joins even if `stop()` was missed. |

**Coverage result:** all 17 rows have at least one tag in the source. No row is
unmapped.

## Extra, beyond the required coverage

- `include/DistributionQueue.h:11` - `[Data structure: binary heap]`. A
  max-heap written by hand rather than `std::priority_queue`, ordering families
  by vulnerability; it uses `Family::operator<` (`src/DistributionQueue.cpp:6`).
- `include/Debug.h:7` - the `RELIEF_LIFECYCLE` macro behind `make debug`.
- Atomic file replacement in `src/CsvStore.cpp:52-92` (temp file, backup and
  rename) goes past what the syllabus asks.

## Compiler evidence that these constructs are load-bearing

Two constructs were removed on purpose, and the compiler's real output was
captured. **Both outputs below are Apple clang's wording**; the error text from
GNU g++ differs and is not shown. The project itself has been built and tested
with GNU g++ 15.2.0 (see `docs/VERIFICATION.md`), but no MinGW/Windows build has
been exercised.

**Virtual base (`virtual` removed from `Resident` and `Volunteer`):**

    src/Adult.cpp:6:7: error: type 'Person' is not a direct or virtual base of 'Adult'
    src/ResidentVolunteer.cpp:44:12: error: non-static member 'getName' found in
        multiple base-class subobjects of type 'Person':
          class ResidentVolunteer -> Adult -> Resident -> Person
          class ResidentVolunteer -> Volunteer -> Person
    src/ResidentVolunteer.cpp:48:51: error: ambiguous conversion from derived
        class 'ResidentVolunteer' to base class 'Person'

The first error also appeared for `Child`, `Infant` and `Elderly`. Without
`virtual`, a `ResidentVolunteer` holds two `Person` subobjects, so naming
`Person` or calling `getName` is ambiguous.

**Friend class (`friend class ReportGenerator;` deleted from `Camp.h`):** the
build failed with 20 errors, the first being:

    src/ReportGenerator.cpp:87:38: error: 'shelters' is a private member of 'Camp'
    include/Camp.h:48:26: note: declared private here

with the same error for `stock`, `id`, `name`, `district`, `occupancy` and
`capacity`. (The line numbers in that capture are from the code as it was then;
`ReportGenerator.cpp` and `Camp.h` have changed since, so do not look for them at
those exact lines. The current friend declaration is `include/Camp.h:93`.)

Both experiments were reverted, and the full suite passed afterwards.

## Where the report and the code disagreed

A brief for this document expected three `[Exception handling]` tags in
`src/ConsoleApp.cpp`. The source has two (lines 428 and 692), so the table cites
two. The per-handler recovery at line 430 sits under the line-428 tag.

## Tag index (checked by `sh tools/check_mapping.sh`)

Every tag comment in the source, generated by `sh tools/collect_tags.sh`. The
check script fails if this list and the source disagree, so citations of tagged
lines cannot go stale unnoticed. The table also cites a few untagged lines (for
example `src/Family.cpp:148`, `src/Camp.cpp:106-127`, `src/Stock.cpp:67`,
`src/Allocation.cpp:19`, `src/CsvStore.cpp:196`); those are only checked to lie
inside their file, so they could drift. Run both from the repository root.

<!-- tag-index-begin -->
```
include/Adult.h:6: [Single inheritance] A camp resident aged 13-59, drawing the full adult
include/Adult.h:8: [Hierarchical inheritance] Infant, Child and Adult all derive from Resident.
include/Adult.h:17: [Overriding a pure virtual] still virtual, so Elderly can override again
include/Algorithms.h:6: [Function template] + [Pointer to member function]
include/Algorithms.h:6: [Pointer to member function]
include/Allocation.h:26: [Class to class conversion] an Allocation can stand in for the Ration it
include/Allocation.h:30: [Friend function] Two allocations are "the same claim" when the same
include/Autosaver.h:27: [Multithreading]
include/Autosaver.h:29: [Mutex]
include/Autosaver.h:32: [Atomic variable]
include/Autosaver.h:38: [Condition variable]
include/Camp.h:16: [Nested class] A shelter has no meaning outside the camp that contains
include/Camp.h:37: [Static data member]
include/Camp.h:57: [Static member function]
include/Camp.h:70: [Objects as arguments] the camp is told about a whole family at once
include/Camp.h:74: [Unary operators] one more, or one fewer, person in the camp
include/Camp.h:93: [Friend class] ReportGenerator reads the camp's private members
include/Child.h:6: [Single inheritance] A camp resident aged 3-12. Children draw a reduced
include/Child.h:8: [Hierarchical inheritance] Infant, Child and Adult all derive from Resident.
include/Child.h:16: [Overriding a pure virtual]
include/Csv.h:12: [Static member functions] no Csv object is ever needed: every helper is a
include/CsvStore.h:43: [File handling] One file per kind of record, each with a header row.
include/CsvStore.h:94: [Runtime polymorphism from data] turns a type tag back into the right
include/Date.h:19: [Const member functions]
include/Date.h:26: [Returning objects]
include/Date.h:33: [Friend functions] free-standing operators
include/Date.h:38: [Binary operator] days between
include/Debug.h:7: [Object lifecycle tracing]
include/DistributionQueue.h:11: [Data structure: binary heap]
include/Elderly.h:8: [Multilevel inheritance]
include/Exceptions.h:9: [Custom exception base class] Everything this program throws derives from
include/Exceptions.h:17: [Overriding a virtual]
include/Exceptions.h:20: [Derived exception] Thrown by Camp::admit and Camp::Shelter::occupy.
include/Exceptions.h:33: [Derived exception] Thrown by Stock::take and operator-(Stock, Allocation).
include/Exceptions.h:45: [Derived exception] Thrown by DistributionLog::record when a family tries to
include/Exceptions.h:56: [Derived exception] Thrown by the CSV layer, with the file and line number
include/Family.h:17: [Dynamic memory]
include/Family.h:18: [Array of dynamic objects]
include/Family.h:24: [Static data member]
include/Family.h:41: [Default constructor]
include/Family.h:43: [Parameterised constructor]
include/Family.h:45: [Copy constructor]
include/Family.h:47: [Copy assignment operator]
include/Family.h:49: [Destructor]
include/Family.h:52: [Static member function]
include/Family.h:70: [Runtime polymorphism]
include/Family.h:72: [Returning objects]
include/Family.h:76: [Pointer to member function] sorting in Task 15.
include/Family.h:83: [Class to class conversion] a Family converts to the printable card an
include/Family.h:88: [Friend functions]
include/FamilySorter.h:11: [Pointer to member function] Metric points at one of Family's metric
include/HealthMonitor.h:34: [Static data members] these are the camp's norms, not one monitor's settings
include/HealthMonitor.h:52: [Static member functions]
include/Infant.h:6: [Single inheritance] A camp resident aged 0-2. Infants need baby food and
include/Infant.h:8: [Hierarchical inheritance] Infant, Child and Adult all derive from Resident.
include/Infant.h:16: [Overriding a pure virtual]
include/Person.h:8: [Abstract class] the root of the whole people hierarchy. It can never be
include/Person.h:11: [Static data members] one counter shared by every Person ever created
include/Person.h:28: [Default constructor]
include/Person.h:29: [Parameterised constructor]
include/Person.h:30: [Copy constructor]
include/Person.h:31: [Virtual destructor]
include/Person.h:33: [Static member functions]
include/Person.h:39: [Const member functions] every getter promises not to modify the object
include/Person.h:52: [Pure virtual functions] -> runtime polymorphism
include/Person.h:59: [Friend function] printing is not a member, so it reads as `os << person`
include/Ration.h:12: [Basic type to class type] non-explicit on purpose
include/Ration.h:15: [Copy constructor]
include/Ration.h:19: [Const member function]
include/Ration.h:26: [Binary operator] read access
include/Ration.h:31: [Friend functions] symmetric arithmetic reads better as free functions
include/RationNorms.h:11: [Static data members] shared by every object, defined once in the .cpp
include/RationNorms.h:20: [Static member functions] no object needed to read a norm
include/Registry.h:7: [Class template] One store for every kind of entity that has an ID. Camps,
include/Registry.h:36: [Const member function] finding does not change the registry
include/ReportGenerator.h:21: [Constant objects]
include/Resident.h:10: [Virtual base class] `virtual public Person` means a ResidentVolunteer, which
include/ResidentVolunteer.h:10: [Multiple inheritance]
include/ResidentVolunteer.h:12: [Hybrid inheritance]
include/Stock.h:17: [Static data member] the answer when nothing is being consumed
include/Stock.h:36: [Class to basic type conversion] `double d = stock;` gives the days of
include/Stock.h:44: [Object as argument]
include/Stock.h:46: [Friend binary operators]
include/TransferAdvisor.h:60: [Objects as arguments] [Returning objects]
include/TransferAdvisor.h:60: [Returning objects]
include/Volunteer.h:10: [Virtual base class] see the note in Resident.h - this is the other half of
include/Volunteer.h:29: [Runtime polymorphism] overrides of the pure virtuals
src/Adult.cpp:10: [Virtual base initialisation]
src/Adult.cpp:29: [Returning objects] the ration is built and returned by value
src/Child.cpp:10: [Virtual base initialisation]
src/ConsoleApp.cpp:428: [Exception handling] a full camp is expected, so it is handled here where
src/ConsoleApp.cpp:655: [Multithreading] start autosaving every kAutosaveSeconds, sharing dataMutex
src/ConsoleApp.cpp:692: [Exception handling] every error path in the program ends here.
src/DistributionQueue.cpp:6: [Operator overloading in use] `*b < *a` means "a outranks b", because
src/Family.cpp:26: [Copy constructor]
src/Family.cpp:35: [Copy assignment operator]
src/Family.cpp:51: [Destructor]
src/Family.cpp:59: [Dynamic memory allocation]
src/Family.cpp:85: [Dynamic memory allocation]
src/HealthMonitor.cpp:43: [Operator overloading in use] Date::operator- gives the age in days
src/Infant.cpp:10: [Virtual base initialisation]
src/Person.cpp:4: [Static data members] definitions live in exactly one .cpp file
src/ReportGenerator.cpp:50: [Friend class in use] camp.stock is a PRIVATE member of Camp.
src/ReportGenerator.cpp:100: [Friend class in use] camp.id, camp.name, camp.district, camp.occupancy,
src/Resident.cpp:9: [Virtual base initialisation]
src/ResidentVolunteer.cpp:5: [Virtual base initialisation]
src/ResidentVolunteer.cpp:18: [Virtual base initialisation]
src/ResidentVolunteer.cpp:35: [Ambiguity resolution]
src/TransferAdvisor.cpp:78: [Objects as arguments] [Returning objects]
src/TransferAdvisor.cpp:78: [Returning objects]
src/Volunteer.cpp:10: [Virtual base initialisation]
```
<!-- tag-index-end -->
