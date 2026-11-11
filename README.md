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

    make                    # builds bin/relief
    ./bin/relief            # uses ./data/ (run from the repository root)
    ./bin/relief <dir>      # uses the CSV files in <dir> instead

    make test       # builds and runs the test suite (1923 assertions)
    make debug      # builds with -DRELIEF_DEBUG: object create/copy/destroy
                    #   messages are printed (the destructor lifecycle)
    make seed       # builds bin/seed and regenerates data/ (overwrites it)
    make clean

The app does not create a missing data directory: a save into one reports the
failure instead of crashing, but the directory must exist.

CMake works too:

    cmake -S . -B build/cmake && cmake --build build/cmake

## Before you demo: start from a fresh dataset

**Every demo run must start from a fresh dataset**, because the app rewrites the
CSV files in its data directory on every autosave and on exit. After one full
pass, camp 2's rice is no longer low (demo step 5 shows no low-stock flag and no
transfer) and every camp-1 family has already claimed today (step 4 prints
"17 BLOCKED, 0 served"). Two safe ways:

    make seed                                   # regenerates data/ (byte-deterministic)
    # or run against a throwaway copy and leave data/ untouched:
    mkdir -p /tmp/demo && cp data/*.csv /tmp/demo && ./bin/relief /tmp/demo

`bash tests/demo_smoke.sh` is the automated end-to-end check (it drives the
binary through a short session on a temporary copy of `data/`). It needs
`bin/relief` to exist, so run `make` first; `make test` does not build it.

## The eight-step demo

The demo date is fixed at 2026-07-14. Menu numbers to press (`./bin/relief`):

1. **Start.** The app loads the seeded data and shows the banner.
2. **Register a family** (`2`) with an infant and an elderly member. The ration and vulnerability score are computed.
3. **Admit to a full camp** (`3`). Camp 2 is full, so the overflow exception is caught and other camps are suggested; admit to one of those.
4. **Distribute rations** (`4`), then run it again (`4`). The second claim the same day is blocked.
5. **Stock** (`5`) shows days of supply with rice critical at camp 2; **transfer suggestions** (`7`) proposes moving rice from camp 3.
6. **Log health cases** (`8`) for camp 2, repeating until the outbreak alert fires.
7. **Daily report** (`10`), and answer `y` to export it as `data/report_<date>_camp<id>.csv` (git-ignored).
8. **Quit** (`0`). Autosave runs; restart and the data is intact.

The key sequence is `2, 3, 4, 5, 7, 8, 10, 0` (with `4` and `8` repeated); a full
piped-input run with output is recorded in `docs/VERIFICATION.md`, section 8.

**Seva First angles.** Affordability: it runs offline on any ordinary laptop with
no server, no network and no licence. Scalability: another district is a
different seed dataset in `data/`, not different code.

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
- `docs/superpowers/specs/2026-11-11-relief-camp-manager-design.md` is the design spec.
- `tests/demo_smoke.sh` is the automated end-to-end check.
- `docs/PLAN.md` is the task-by-task implementation plan this was built from,
  kept for reference as process history (it is addressed to the implementer, not
  to a reader of the project).

## Layout

`include/` headers, `src/` sources, `tests/` the self-written assert-based test
runner (no framework), `data/` the CSV files, `docs/` documentation.
