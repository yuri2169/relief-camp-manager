import { site } from "@/content/site";
import { Section } from "./Section";

export function Inside() {
  const { title, rows } = site.inside;
  return (
    <Section id="inside" title={title}>
      <dl>
        {rows.map((row) => (
          <div
            key={row.label}
            className="flex flex-wrap items-baseline justify-between gap-x-6 gap-y-1 border-t-3 border-ink py-4"
          >
            <dt className="text-callout font-extrabold">{row.label}</dt>
            <dd className="font-bold">{row.value}</dd>
          </div>
        ))}
      </dl>
    </Section>
  );
}
