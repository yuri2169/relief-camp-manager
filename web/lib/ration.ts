// The ration norms and vulnerability scores, copied from the C++ program's
// RationNorms.cpp and the vulnerabilityScore() of each Person subclass. Keep
// the two in step: these are the figures the console app computes.

import type { ItemId } from "@/content/demo-data";

export type Category = "infant" | "child" | "adult" | "elderly";

/** The age bands the Person subclasses are built on. */
export function categoryFor(age: number): Category {
  if (age <= 2) return "infant";
  if (age <= 12) return "child";
  if (age <= 59) return "adult";
  return "elderly";
}

/** Only working-age adults are offered a camp duty, as in the console app. */
export function canTakeDuty(age: number): boolean {
  return categoryFor(age) === "adult";
}

type Norm = Partial<Record<ItemId, number>>;

const norms: Record<Category, Norm> = {
  adult: { rice: 0.4, dal: 0.06, oil: 0.03, salt: 0.01, ors: 0.1 },
  child: { rice: 0.2, dal: 0.04, oil: 0.02, salt: 0.008, ors: 0.15 },
  infant: { oil: 0.01, salt: 0.002, ors: 0.25, "baby-food": 0.1 },
  elderly: { rice: 0.3, dal: 0.06, oil: 0.035, salt: 0.01, ors: 0.2 },
};

const PREGNANCY_RICE = 0.1;
const LACTATION_OIL = 0.01;

const baseScore: Record<Category, number> = {
  infant: 40,
  elderly: 35,
  child: 20,
  adult: 10,
};

export type Member = {
  name: string;
  age: number;
  pregnant: boolean;
  disabled: boolean;
  lactating: boolean;
  /** An adult who also works a camp duty becomes a ResidentVolunteer. */
  duty: boolean;
};

/**
 * A resident who also volunteers inherits Person once, through two paths, and
 * the class resolves the ambiguity in favour of Adult. So a camp duty changes
 * the person's type but neither their ration nor their score.
 */
export function typeOf(member: Member): Category | "resident-volunteer" {
  const category = categoryFor(member.age);
  if (category === "adult" && member.duty) return "resident-volunteer";
  return category;
}

export function rationFor(member: Member): Norm {
  const category = categoryFor(member.age);
  const ration: Norm = { ...norms[category] };
  if (member.pregnant) ration.rice = (ration.rice ?? 0) + PREGNANCY_RICE;
  if (member.lactating) ration.oil = (ration.oil ?? 0) + LACTATION_OIL;
  return ration;
}

export function scoreFor(member: Member): number {
  const flags =
    (member.pregnant ? 25 : 0) + (member.disabled ? 20 : 0) + (member.lactating ? 15 : 0);
  return baseScore[categoryFor(member.age)] + flags;
}

export type FamilyTotals = {
  ration: { item: ItemId; amount: number }[];
  score: number;
  counts: Record<Category, number>;
};

export function totalsFor(members: Member[]): FamilyTotals {
  const totals = new Map<ItemId, number>();
  const counts: Record<Category, number> = { infant: 0, child: 0, adult: 0, elderly: 0 };
  let score = 0;

  for (const member of members) {
    counts[categoryFor(member.age)] += 1;
    score += scoreFor(member);
    for (const [item, amount] of Object.entries(rationFor(member)) as [ItemId, number][]) {
      totals.set(item, (totals.get(item) ?? 0) + amount);
    }
  }

  const order: ItemId[] = ["rice", "dal", "oil", "salt", "ors", "baby-food"];
  return {
    ration: order
      .filter((item) => (totals.get(item) ?? 0) > 0)
      .map((item) => ({ item, amount: totals.get(item) as number })),
    score,
    counts,
  };
}

/** Trailing zeros removed, so 0.70 reads as 0.7 and 0.022 keeps its digits. */
export function amount(value: number): string {
  return Number(value.toFixed(3)).toString();
}
