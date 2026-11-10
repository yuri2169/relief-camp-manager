# Project Context: Offline Relief Camp Manager (Assam Floods)

## Who and why
- Team of 2, 3rd-semester B.Tech CSE students at Delhi Technological University (DTU).
- This is our **Object Oriented Programming (CS 203) course project**. The professor's rule: a simple project that works **purely on the OOP syllabus, in C++**. The project will be graded mainly on how clearly it shows OOP concepts.
- It is also our entry to the **Seva First Innovation Challenge 2026, Theme 2 (Local Community Level Innovations), category REG-009-OP: Safety & Disaster Management**. Judges reward a real local problem, an affordable and implementable solution, scalability to other regions, and a clear path from proof of concept to a validated, demonstrable solution.

## The problem
Every monsoon, floods in Assam push large numbers of people into temporary relief camps run by district officials and volunteers. At camp level, families, rations and supplies are still largely tracked on paper registers. As a result:
- stock runs out without warning
- some families get rations twice while others are missed
- vulnerable members (infants, pregnant women, elderly, disabled) are not prioritised
- daily reports to the circle office are slow and error-prone
- internet is often down in flood-hit areas, so online tools cannot be relied on

## The solution
An **offline-first, console-based relief camp management system** in C++ that runs on one low-cost laptop. No internet, no database server, and no external libraries. Data is saved to plain CSV files so it survives a power cut.

### Core features (prototype scope)
1. **Camp registry**: camps with capacity, current occupancy and an overflow alert.
2. **Family registration**: a family has members, and each member is a typed person (Infant, Child, Adult, Elderly). Flags for pregnant, disabled and lactating.
3. **Ration entitlement**: daily ration per family, computed from its members. Each person type overrides its own `rationNeed()`.
4. **Distribution log**: record a distribution, and reject duplicate claims for the same family on the same day, across all camps.
5. **Stock management**: stock in and out per item (rice, dal, oil, salt, ORS, baby food, tarpaulin). Projects **days of supply left** from current stock and current daily demand.
6. **Inter-camp transfer suggestion**: find a camp with surplus of an item and a camp that is short, and suggest a transfer.
7. **Health log and outbreak alert**: log symptoms per camp per day. Alert when diarrhoea or fever cases cross a threshold in a 3-day window.
8. **Priority queue**: order families for distribution by vulnerability score.
9. **Daily report**: a camp summary printed to the console and exported to CSV for officials.
10. **Persistence**: save and load everything from CSV. A background thread autosaves every N seconds.

### Out of scope for the prototype
GUI, networking, login and authentication, multi-language UI, SMS.

## Hard technical constraints
- **C++17**, standard library only. Must build with `g++` on Linux, macOS and Windows (MinGW). Provide a `Makefile` and a `CMakeLists.txt`.
- Console menu UI. It must be clear and demo-friendly.
- Keep the code **readable for a 3rd-semester student**: no template metaprogramming, no clever modern tricks that hide the OOP concepts. Using STL containers internally is fine, but the syllabus constructs below must be **written by us and clearly visible**.
- One class per header/source pair. Folders: `include/`, `src/`, `tests/`, `data/`.
- Seed `data/` with realistic sample data for the demo: 3 camps in one Assam district (for example, Morigaon or Dhemaji) and about 40 families.
- Tests use a tiny self-written assert-based test runner in `tests/`. Do not download any framework.

## OOP syllabus coverage (mandatory: each item must appear naturally in the design)
| Concept | Where it should appear |
|---|---|
| Classes, objects, constructors (default, parameterised, copy), destructors | All entities. Print a message from at least one destructor in debug mode to show the object lifecycle |
| Constant objects and const member functions | All getters; reports take `const` references |
| Static data members and static member functions | Auto-increment ID generators; the ration-norm table |
| Friend function and friend class | `ReportGenerator` as a friend of `Camp`; `operator<<` as friend functions |
| Objects as arguments and returning objects | Transfers, stock arithmetic |
| Nested class | `Camp::Shelter` (tent or hall inside a camp) |
| Dynamic memory and dynamic objects, arrays of objects | Persons created with `new`, owned and freed correctly (show rule-of-three on one class, and use smart pointers elsewhere) |
| Pointers to members | Sort or filter families by a chosen field via a pointer-to-member-function |
| Operator overloading: unary, binary, friend-based | `Stock + Stock`, `Stock - Allocation`, `++`/`--` occupancy, `<` for priority, `==` for duplicate check, `<<` for printing |
| Type conversion: class to basic, basic to class, class to class | `Stock` to `double` (days of supply); `int` to `Ration`; `Family` to `RationCard` |
| Inheritance: single, multilevel, hierarchical, multiple, hybrid | `Person` → `Adult` → `Elderly`; `Person` → `Infant`/`Child`/`Adult`; `Volunteer` + `Resident` → `ResidentVolunteer` |
| Virtual base class | `Person` as a virtual base of `Volunteer` and `Resident` (solves the diamond) |
| Abstract class, pure virtual functions, runtime polymorphism | `Person::rationNeed()`, `Person::vulnerabilityScore()`, `Person::describe()` called through base pointers |
| Templates (class and function) | `Registry<T>` (store, find by ID, remove); a `findMax` function template |
| Exception handling with custom exception classes | `OverCapacityException`, `InsufficientStockException`, `DuplicateClaimException`, `FileFormatException`, all derived from a base `ReliefException` |
| File handling | CSV save and load for every registry; report export |
| Multithreading | Background autosave thread with a mutex; clean shutdown |

Also generate `docs/OOP_MAPPING.md`, which lists every concept above with file name and line references, so the professor can verify coverage quickly.

## Demo script (what must work end to end)
1. Start the app. It loads seeded data from `data/`.
2. Register a new family with an infant and an elderly member. Show the computed ration and vulnerability score.
3. Try to admit them to a full camp. The overflow exception is caught, and the app suggests another camp.
4. Distribute rations. Then try again the same day, and the duplicate claim is blocked.
5. Show stock and days-of-supply. One item is critical, so the app suggests a transfer from another camp.
6. Log health cases, which triggers an outbreak alert.
7. Generate and export the daily report.
8. Quit. Autosave runs, and on restart the data is intact.
