// Wrapper for every landing section below the hero: a thick rule, a big
// title, then content. Use tone="field" at most once per page.
export function Section({
  id,
  title,
  tone = "paper",
  children,
}: {
  id: string;
  title: string;
  tone?: "paper" | "field";
  children: React.ReactNode;
}) {
  return (
    <section
      id={id}
      aria-labelledby={`${id}-title`}
      className={`${tone === "field" ? "bg-field" : "bg-paper"} scroll-mt-4 px-5 py-14 md:px-8 md:py-20`}
    >
      <div className="border-t-3 border-ink pt-5">
        <h2 id={`${id}-title`} className="max-w-[16ch] text-title font-extrabold">
          {title}
        </h2>
        <div className="mt-8 md:mt-12">{children}</div>
      </div>
    </section>
  );
}
