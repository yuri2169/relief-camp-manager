// The Morigaon demo dataset, taken from the C++ program's committed seed
// files (data/camps.csv, data/stock.csv, data/shelters.csv, data/health.csv).
// Quantities are exactly as stored there. Daily demand is what the program
// derives from each camp's registered members, so quantity / dailyDemand
// reproduces the days of supply the console app prints.

export type ItemId = "rice" | "dal" | "oil" | "salt" | "ors" | "baby-food" | "tarpaulin";

export type StockLine = {
  item: ItemId;
  /** Units on hand. */
  quantity: number;
  /** Units the camp issues per day. Zero means the item is not rationed. */
  dailyDemand: number;
};

export type HealthLine = {
  symptom: string;
  /** Cases inside the rolling three-day window. */
  cases: number;
  /** Cases at which the outbreak alert fires. */
  threshold: number;
};

export type Camp = {
  /** Slug used in URLs. */
  id: string;
  /** The camp number the console app shows. */
  number: number;
  name: string;
  district: string;
  capacity: number;
  occupancy: number;
  shelters: number;
  stock: StockLine[];
  health: HealthLine[];
};

/** The demo is fixed to this date so the data always tells the same story. */
export const TODAY = "2026-07-14T00:00:00Z";

export const itemNames: Record<ItemId, { name: string; unit: string }> = {
  rice: { name: "Rice", unit: "kg" },
  dal: { name: "Dal", unit: "kg" },
  oil: { name: "Oil", unit: "litre" },
  salt: { name: "Salt", unit: "kg" },
  ors: { name: "ORS", unit: "packets" },
  "baby-food": { name: "Baby food", unit: "kg" },
  tarpaulin: { name: "Tarpaulin", unit: "sheets" },
};

export const camps: Camp[] = [
  {
    id: "bhuragaon",
    number: 1,
    name: "Bhuragaon HS School",
    district: "Morigaon",
    capacity: 180,
    occupancy: 48,
    shelters: 3,
    stock: [
      { item: "rice", quantity: 170.4, dailyDemand: 14.2 },
      { item: "dal", quantity: 27.12, dailyDemand: 2.26 },
      { item: "oil", quantity: 14.76, dailyDemand: 1.23 },
      { item: "salt", quantity: 4.848, dailyDemand: 0.404 },
      { item: "ors", quantity: 81, dailyDemand: 6.75 },
      { item: "baby-food", quantity: 8.4, dailyDemand: 0.7 },
      { item: "tarpaulin", quantity: 40, dailyDemand: 0 },
    ],
    health: [{ symptom: "Fever", cases: 2, threshold: 15 }],
  },
  {
    id: "laharighat",
    number: 2,
    name: "Laharighat Community Hall",
    district: "Morigaon",
    capacity: 90,
    occupancy: 88,
    shelters: 2,
    stock: [
      { item: "rice", quantity: 44.4, dailyDemand: 22.2 },
      { item: "dal", quantity: 31.36, dailyDemand: 3.92 },
      { item: "oil", quantity: 16.24, dailyDemand: 2.03 },
      { item: "salt", quantity: 5.888, dailyDemand: 0.736 },
      { item: "ors", quantity: 100.8, dailyDemand: 12.6 },
      { item: "baby-food", quantity: 3.2, dailyDemand: 0.4 },
      { item: "tarpaulin", quantity: 40, dailyDemand: 0 },
    ],
    health: [{ symptom: "Diarrhoea", cases: 7, threshold: 10 }],
  },
  {
    id: "mayong",
    number: 3,
    name: "Mayong Embankment Tents",
    district: "Morigaon",
    capacity: 120,
    occupancy: 29,
    shelters: 3,
    stock: [
      { item: "rice", quantity: 156.6, dailyDemand: 8.7 },
      { item: "dal", quantity: 12.42, dailyDemand: 1.38 },
      { item: "oil", quantity: 6.66, dailyDemand: 0.74 },
      { item: "salt", quantity: 2.214, dailyDemand: 0.246 },
      { item: "ors", quantity: 36, dailyDemand: 4 },
      { item: "baby-food", quantity: 3.6, dailyDemand: 0.4 },
      { item: "tarpaulin", quantity: 40, dailyDemand: 0 },
    ],
    health: [{ symptom: "Skin infection", cases: 1, threshold: 20 }],
  },
];

export function findCamp(id: string): Camp | undefined {
  return camps.find((camp) => camp.id === id);
}
