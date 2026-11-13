import { site } from "@/content/site";
import { Section } from "./Section";

export function Problem() {
  const { title, paragraphs, pull } = site.problem;
  return (
    <Section id="problem" title={title}>
      <div className="grid gap-10 md:grid-cols-[1fr_minmax(0,22rem)] md:gap-16">
        <div className="max-w-[58ch] space-y-5 text-lede">
          {paragraphs.map((paragraph) => (
            <p key={paragraph}>{paragraph}</p>
          ))}
        </div>
        <p className="self-start bg-field p-5">
          <span className="block text-figure font-extrabold">{pull.figure}</span>
          <span className="mt-1 block font-bold">{pull.label}</span>
        </p>
      </div>
    </Section>
  );
}
