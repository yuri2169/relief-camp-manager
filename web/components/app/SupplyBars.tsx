// Days of supply for each rationed item, drawn with divs. The thinnest item
// is filled with the field colour and keeps its text label, so the signal is
// not colour alone.
export function SupplyBars({
  bars,
  caption,
  label,
}: {
  bars: { name: string; days: number }[];
  caption: string;
  label: string;
}) {
  const max = Math.max(...bars.map((bar) => bar.days), 1);
  const thinnest = bars.reduce((acc, bar, index) => (bar.days < bars[acc].days ? index : acc), 0);

  return (
    <figure>
      <div role="img" aria-label={label} className="flex h-44 items-end gap-2 border-b-3 border-ink">
        {bars.map((bar, index) => (
          <div
            key={bar.name}
            className={
              index === thinnest
                ? "flex-1 border-3 border-b-0 border-ink bg-field"
                : "flex-1 bg-ink"
            }
            style={{ height: `${Math.max((bar.days / max) * 100, 2)}%` }}
          />
        ))}
      </div>
      <div aria-hidden className="flex gap-2">
        {bars.map((bar, index) => (
          <p
            key={bar.name}
            className={`flex-1 pt-1.5 text-sm leading-tight ${index === thinnest ? "font-extrabold" : "font-bold opacity-70"}`}
          >
            {bar.name}
          </p>
        ))}
      </div>
      <figcaption className="mt-3 text-sm font-bold">{caption}</figcaption>
    </figure>
  );
}
