/** The warning threshold is a whole number of days from 1 to 60. */
export function isValidThreshold(value: string): boolean {
  const text = value.trim();
  if (!/^\d+$/.test(text)) return false;
  const days = Number(text);
  return days >= 1 && days <= 60;
}
