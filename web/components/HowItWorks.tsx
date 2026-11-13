import { site } from "@/content/site";
import { Section } from "./Section";

export function HowItWorks() {
  const { title, steps } = site.how;
  return (
    <Section id="how" title={title}>
      <ol>
        {steps.map((step, index) => (
          <li
            key={step.title}
            className="grid grid-cols-[3.5rem_1fr] gap-x-4 border-t-3 border-ink py-5 md:grid-cols-[8rem_1fr_1.4fr] md:gap-x-8 md:py-7"
          >
            <span aria-hidden className="text-figure font-extrabold">
              {index + 1}
            </span>
            <h3 className="self-center text-callout font-extrabold">{step.title}</h3>
            <p className="col-start-2 mt-2 max-w-[46ch] md:col-start-3 md:mt-0 md:self-center">
              {step.body}
            </p>
          </li>
        ))}
      </ol>
    </Section>
  );
}
