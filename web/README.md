# Relief Camp Manager — web interface

A browser view of the same Morigaon dataset the console program uses, so the
project can be shown to an audience without a terminal.

This is a separate presentation layer. It has its own toolchain, nothing in it
is compiled or tested by the `Makefile` in the repository root, and the two
halves share no code — they share a dataset, and this half recomputes its
figures by the same rules so the two agree to the decimal.

## Run it

Needs Node.js 20 or newer. The same two commands work on macOS, Linux and
Windows:

```bash
npm install
npm run dev
```

Then open http://localhost:3000. The app screens are under
http://localhost:3000/app.

For a production build:

```bash
npm run build
npm start
```

## Routes

| Route | What it shows |
|---|---|
| `/` | Landing page: headline, the numbers, the problem, how it works, the demo |
| `/app` | Overview: items below three days, a tile per camp, the transfer panel |
| `/app/camps` | All three camps with occupancy, days of supply and status |
| `/app/camps/[id]` | One camp: days of supply, facts, stock table, supply chart, health log |
| `/app/register` | Register a family, with the ration card computed as you type |
| `/app/settings` | Settings form |
| `/app/states` | The empty, loading and error states in one place |

## Where the figures come from

Nothing here is invented. The dataset and the arithmetic mirror the console
program:

- `content/demo-data.ts` holds the three camps, their stock and their health
  rows, taken from the committed seed files in `../data/`. Quantity divided by
  daily demand reproduces the days of supply the console program prints.
- `lib/supply.ts` computes days of supply and the inter-camp transfers by the
  same rule as `../src/TransferAdvisor.cpp`, including the guard that stops one
  donor's spare stock being pledged to two receivers in a single pass. That is
  why the panel proposes 22.2 kg of rice from Mayong to Laharighat.
- `lib/ration.ts` carries the ration norms from `../src/RationNorms.cpp` and the
  vulnerability scores from each `Person` subclass. Registering one infant, one
  elderly member and one adult gives a score of 85, the figure the console
  program prints on its ration card.

Ticking the camp duty on a working-age adult turns them into a resident
volunteer, which the card explains on screen.

## Nothing is written

The interface is read-only over a fixed snapshot. It does not touch the CSV
files, and the demo date is pinned to 14 July 2026 so the figures never drift.
The buttons that look like they commit something say so when pressed.

## Where things live

- `content/site.ts` — every string on the site. Components contain no copy.
- `content/demo-data.ts` — the camp records.
- `styles/tokens.css` — colours and type sizes. Change `--color-field` to
  recolour the whole site.
- `components/` — landing sections. `components/app/` — app screens.
- `lib/` — the supply, ration and validation functions.
