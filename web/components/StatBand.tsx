import { site } from "@/content/site";

export function StatBand() {
  return (
    <dl className="on-ink grid bg-ink text-paper md:grid-cols-3">
      {site.stats.map((stat) => (
        <div
          key={stat.label}
          className="flex flex-col-reverse border-t border-ink-rule px-5 py-6 first:border-t-0 md:border-l md:border-t-0 md:px-8 md:first:border-l-0"
        >
          <dt className="text-small opacity-85">{stat.label}</dt>
          <dd className="text-figure font-extrabold text-field">{stat.figure}</dd>
        </div>
      ))}
    </dl>
  );
}
