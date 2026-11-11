# Offline Relief Camp Manager

An offline-first, console-based relief camp management system in C++17 for flood
relief camps in Assam. It runs on one low-cost laptop with no internet, no
database server and no external libraries. Data is saved to plain CSV files so
it survives a power cut.

Built as the project for **Object Oriented Programming (CS 203)**, and entered in
the **Seva First Innovation Challenge 2026**, Theme 2 (Local Community Level
Innovations), category REG-009-OP: Safety & Disaster Management.

Every monsoon, camp registers are still kept on paper: stock runs out without
warning, some families are rationed twice while others are missed, vulnerable
people are not prioritised, and daily reports are slow. This program tracks
camps, families (each member a typed Infant, Child, Adult or Elderly person),
ration entitlement, a duplicate-proof distribution log, stock with days of supply
left, inter-camp transfer suggestions, a health log with outbreak alerts, a
vulnerability-ordered queue, and a daily report exported to CSV. A background
thread autosaves every few seconds.

## Build and run

Needs a C++17 compiler and `make`. Standard library only.

    make            # builds bin/relief
    ./bin/relief    # run from the repository root so it finds data/

    make test       # builds and runs the test suite (1923 assertions)
    make debug      # builds with -DRELIEF_DEBUG: object create/copy/destroy
                    #   messages are printed (the destructor lifecycle)
    make seed       # builds bin/seed and regenerates data/ (overwrites it)
    make clean

CMake works too:

    cmake -S . -B build/cmake && cmake --build build/cmake

## The dataset

`data/` holds nine committed CSV files: the Morigaon demo dataset (three camps,
about 40 families). A fresh clone runs the demo with no setup. Running
`make seed` regenerates the files and **overwrites** them.

The data is rigged so the demo has a story. Camp 2 is nearly full (88 of 90) and has about
two days of rice left. Camp 3 is the donor, holding about eighteen days of rice
(camp 1 has twelve). Diarrhoea
cases sit at seven, just under the outbreak threshold of ten, and nothing has
been distributed yet on the demo date, 2026-07-14.

## Platform

Built and tested on macOS with **Apple clang 21** (Makefile and CMake) and with
**GNU g++ 15.2.0** (Homebrew), which builds warning-free and passes the full
suite. The Makefile and CMakeLists.txt are written to be portable to Linux and
Windows MinGW, but **MinGW/Windows has not been exercised**, and neither has a
Linux machine. See `docs/VERIFICATION.md` for the recorded output.

## Documentation

- `tools/collect_tags.sh` and `tools/check_mapping.sh` regenerate and verify the
  tag index behind that mapping (`sh tools/check_mapping.sh` from the root).
- `docs/OOP_MAPPING.md` maps every syllabus concept to a file and line, with the
  compiler evidence and an honest note on which constructs are taught rather
  than natural.
- `docs/PLAN.md` is the plan this was built from.

## Layout

`include/` headers, `src/` sources, `tests/` the self-written assert-based test
runner (no framework), `data/` the CSV files, `docs/` documentation.
