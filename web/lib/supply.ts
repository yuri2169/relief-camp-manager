import { camps, itemNames, type Camp, type ItemId, type StockLine } from "@/content/demo-data";

export type Status = "low" | "watch" | "ok";

/**
 * Stock that lasts this many days or fewer is critical, and a transfer tops
 * the camp back up to exactly this many days. The C++ program uses the same
 * figure, which is why it proposes moving 22.2 kg of rice to Laharighat:
 * 3 x 22.2 - 44.4 = 22.2.
 */
export const CRITICAL_DAYS = 3;

/**
 * Stock that lasts this many days or fewer is worth watching. This band is
 * presentation only: the console app flags LOW or nothing.
 */
export const WATCH_DAYS = 10;

/** Days of supply for one line, or null when the item is not rationed. */
export function daysFor(line: StockLine): number | null {
  if (line.dailyDemand <= 0) return null;
  return line.quantity / line.dailyDemand;
}

export function statusOf(days: number | null): Status {
  if (days === null) return "ok";
  if (days <= CRITICAL_DAYS) return "low";
  if (days <= WATCH_DAYS) return "watch";
  return "ok";
}

export type StockRow = {
  item: ItemId;
  name: string;
  unit: string;
  quantity: number;
  dailyDemand: number;
  days: number | null;
  status: Status;
};

export function stockRows(camp: Camp): StockRow[] {
  return camp.stock.map((line) => {
    const days = daysFor(line);
    return {
      item: line.item,
      name: itemNames[line.item].name,
      unit: itemNames[line.item].unit,
      quantity: line.quantity,
      dailyDemand: line.dailyDemand,
      days,
      status: statusOf(days),
    };
  });
}

export type CampSummary = {
  camp: Camp;
  /** The worst rationed item's days of supply. */
  days: number | null;
  /** The item that produced that figure. */
  criticalItem: string | null;
  status: Status;
  free: number;
  /** Rationed items at or below the critical threshold. */
  lowItems: StockRow[];
  /** Symptoms whose case count has reached the outbreak threshold. */
  alerts: string[];
};

export function summarise(camp: Camp): CampSummary {
  const rows = stockRows(camp);
  const rationed = rows.filter((row) => row.days !== null);
  const worst = rationed.reduce<StockRow | null>(
    (acc, row) => (acc === null || (row.days as number) < (acc.days as number) ? row : acc),
    null,
  );
  return {
    camp,
    days: worst ? worst.days : null,
    criticalItem: worst ? worst.name : null,
    status: statusOf(worst ? worst.days : null),
    free: camp.capacity - camp.occupancy,
    lowItems: rows.filter((row) => row.status === "low"),
    alerts: camp.health.filter((line) => line.cases >= line.threshold).map((line) => line.symptom),
  };
}

export function allSummaries(): CampSummary[] {
  return camps.map(summarise).sort((a, b) => {
    if (a.days === b.days) return a.camp.number - b.camp.number;
    if (a.days === null) return 1;
    if (b.days === null) return -1;
    return a.days - b.days;
  });
}

export type Transfer = {
  id: string;
  item: ItemId;
  itemName: string;
  unit: string;
  quantity: number;
  from: Camp;
  to: Camp;
  fromDays: number;
  toDays: number;
};

/**
 * The same rule the C++ TransferAdvisor follows. For every camp below the
 * critical threshold it finds the donor with the most days of supply and
 * proposes moving exactly the shortfall. `promised` is what stops one donor's
 * spare stock being pledged to two receivers in the same pass.
 */
export function transfers(): Transfer[] {
  const found: Transfer[] = [];
  const promised = new Map<string, number>();
  const key = (campId: string, item: ItemId) => `${campId}:${item}`;

  for (const to of camps) {
    for (const line of to.stock) {
      if (line.dailyDemand <= 0) continue;
      const toDays = line.quantity / line.dailyDemand;
      if (toDays > CRITICAL_DAYS) continue;

      const needed = CRITICAL_DAYS * line.dailyDemand - line.quantity;
      if (needed <= 0.0001) continue;

      let best: Camp | null = null;
      let bestDays = 0;
      let bestSpare = 0;

      for (const from of camps) {
        if (from.id === to.id) continue;
        const theirs = from.stock.find((other) => other.item === line.item);
        if (!theirs || theirs.dailyDemand <= 0) continue;
        const already = promised.get(key(from.id, line.item)) ?? 0;
        const have = theirs.quantity - already;
        const keep = CRITICAL_DAYS * theirs.dailyDemand;
        const spare = have - keep;
        if (spare <= 0.0001) continue;
        const fromDays = have / theirs.dailyDemand;
        if (fromDays > bestDays) {
          best = from;
          bestDays = fromDays;
          bestSpare = spare;
        }
      }

      if (!best) continue;
      const quantity = Math.min(needed, bestSpare);
      promised.set(key(best.id, line.item), (promised.get(key(best.id, line.item)) ?? 0) + quantity);
      found.push({
        id: `${best.id}-${to.id}-${line.item}`,
        item: line.item,
        itemName: itemNames[line.item].name,
        unit: itemNames[line.item].unit,
        quantity,
        from: best,
        to,
        fromDays: bestDays,
        toDays,
      });
    }
  }

  return found;
}

/** One decimal, the way the console app prints quantities and days. */
export function oneDecimal(value: number): string {
  return value.toFixed(1);
}
