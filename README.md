# Offline Relief Camp Manager

An offline-first relief camp management system for flood relief camps in Assam.
It runs on one low-cost laptop with no internet, no database server and no
external libraries. Data is saved to plain CSV files, so it survives a power cut
and can be opened in any spreadsheet by a camp official who has never seen this
program.

Entered in the **Seva First Innovation Challenge 2026**, Theme 2 (Local
Community Level Innovations), category REG-009-OP: Safety and Disaster
Management.

## The problem

Every monsoon, camp registers are still kept on paper. Stock runs out without
warning because nobody is dividing what is left by how fast it goes. Some
families are rationed twice while others are missed, because a paper register
cannot be searched while a queue is waiting. The people who most need serving
first are served in the order they happen to arrive. And a camp with eighteen
days of rice has no way of knowing that the camp an hour away has two.

## What it does

- **Camps and shelters** with capacity, occupancy and overflow handling.
- **Families** made of typed people — infant, child, adult, elderly, volunteer —
  each drawing a different daily ration.
- **Ration entitlement** computed from relief norms, including pregnancy and
  breastfeeding additions.
- **A duplicate-proof distribution log**, so a family cannot collect twice in
  one day.
- **Stock with days of supply left**, flagged when it falls below three days.
- **Inter-camp transfer suggestions**: which camp can spare what, and how much
  to move.
- **A health log with outbreak alerts** over a rolling three-day window.
- **A vulnerability-ordered queue**, so the most vulnerable are served first.
- **A daily report** exported to CSV.
- **A background autosave thread** that writes every thirty seconds.

## Requirements

A C++17 compiler. Nothing else — the program uses only the C++ standard
library, with no third-party dependencies and no network access at any point.

| Platform | What you need |
|---|---|
| macOS | Xcode command line tools (`xcode-select --install`) |
| Linux | `g++` and `make`, or `cmake` |
| Windows | MinGW-w64 or Visual Studio 2019+, plus CMake |

## Run it on macOS or Linux

```bash
make
./bin/relief
```

`make` produces `bin/relief`. Run it from the repository root so it finds the
`data/` folder, or give it a folder of its own:

```bash
./bin/relief /path/to/another/dataset
```

Other targets:

```bash
make test     # builds and runs the test suite (1923 assertions)
make seed     # regenerates data/ from scratch (overwrites it)
make clean
```

CMake works here too, if you prefer it:

```bash
cmake -S . -B build && cmake --build build
./build/relief
```

## Run it on Windows

Use CMake. The `Makefile` expects a Unix shell, so it only works under Git Bash
or MSYS2; CMake needs no shell at all.

**With Visual Studio:**

```bat
cmake -S . -B build
cmake --build build --config Release
.\build\Release\relief.exe
```

**With MinGW-w64:**

```bat
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\relief.exe
```

Run it from the repository root so it finds `data\`, or pass a folder as the
first argument. To run the tests:

```bat
ctest --test-dir build -C Release --output-on-failure
```

A note on honesty: the program is written in portable standard C++ and includes
no platform-specific headers, and `CMakeLists.txt` handles MSVC's warning flags
and thread linking. It has been built and tested on macOS with Apple clang 21
and GNU g++ 15.2.0. The Windows steps above follow from the same CMake file but
have not been run on a Windows machine.

## Before you demo: start from a fresh dataset

**Every demo run must start from a fresh dataset**, because the program rewrites
the CSV files in its data folder on every autosave and on exit. After one full
pass, camp 2's rice is no longer low and every camp-1 family has already claimed
today, so two of the demo's best moments are gone. Two safe ways:

```bash
make seed
```

Or leave `data/` alone and run against a throwaway copy:

```bash
mkdir -p /tmp/demo && cp data/*.csv /tmp/demo && ./bin/relief /tmp/demo
```

On Windows:

```bat
mkdir %TEMP%\demo && copy data\*.csv %TEMP%\demo && .\build\Release\relief.exe %TEMP%\demo
```

## The eight-step demo

The demo date is fixed at 2026-07-14, so the figures never drift. Menu numbers
to press:

1. **Start.** The program loads the dataset and shows the banner.
2. **Register a family** (`2`) with an infant and an elderly member. The ration
   and vulnerability score are computed from the ages you enter.
3. **Admit to a full camp** (`3`). Camp 2 has two beds free, so admitting three
   people is refused and camps with room are suggested. Admit to one of those.
4. **Distribute rations** (`4`) for camp `1`, then run it again (`4`, camp `1`).
   The second claim on the same day is blocked, family by family.
5. **Stock** (`5`, camp `2`) shows rice at 2.0 days. **Transfer suggestions**
   (`7`) proposes moving 22.2 kg of rice from camp 3. Apply it, then view the
   stock again (`5`, camp `2`) to see the days rise.
6. **Log health cases** (`8`) for camp `2`, repeating until the outbreak alert
   fires on the tenth case in the window.
7. **Daily report** (`10`, camp `2`), answering `y` to export it as
   `report_<date>_camp<id>.csv`.
8. **Quit** (`0`). Autosave writes everything out; restart and the data is
   intact.

The key sequence is `2, 3, 4, 5, 7, 8, 10, 0`, with `4`, `5` and `8` repeated.

`bash tests/demo_smoke.sh` drives the program through a short session
automatically, on a temporary copy of `data/`. It needs `bin/relief` to exist,
so run `make` first.

## The web interface

`web/` holds a browser view of the same dataset, for showing the project
without a terminal. It is a separate presentation layer with its own toolchain;
the `Makefile` does not build or test anything in it, and the two share no code.
See `web/README.md`.

```bash
cd web && npm install && npm run dev
```

## The dataset

`data/` holds nine committed CSV files: the Morigaon demo dataset, with three
camps and about forty families. A fresh clone runs the demo with no setup.

The data is rigged so the demo has a story. Camp 2 is nearly full, at 88 of 90,
and has about two days of rice left. Camp 3 is the donor, holding about eighteen
days of rice, where camp 1 has twelve. Diarrhoea cases sit at seven, just under
the outbreak threshold of ten, and nothing has been distributed yet on the demo
date.

`make seed` regenerates all nine files deterministically and **overwrites** them.

## Layout

```
include/   headers, one per class
src/       sources, plus main.cpp and the seed generator
tests/     the self-written assert-based test runner and the smoke test
data/      the nine CSV files
web/       the browser interface
```

The test suite is hand-written: a small assert runner with a pass-fail tally and
a non-zero exit code on failure, with no external framework, so the project has
no dependencies to install at any point.
