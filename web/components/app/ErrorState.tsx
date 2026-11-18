import { buttonFieldClass } from "@/components/ui";

export function ErrorState({
  title,
  body,
  retryLabel,
  onRetry,
  announce = false,
  headingLevel,
}: {
  title: string;
  body: string;
  retryLabel: string;
  /** Without a retry handler the button is not shown. */
  onRetry?: () => void;
  /** True only when the state replaces a page after a failure, so it is read out. */
  announce?: boolean;
  headingLevel: 1 | 2 | 3;
}) {
  const Heading = `h${headingLevel}` as "h1" | "h2" | "h3";
  return (
    <div role={announce ? "alert" : undefined} className="on-ink bg-ink px-5 py-10 text-paper md:px-8">
      <Heading className="text-title font-extrabold text-field">{title}</Heading>
      <p className="mt-3 max-w-[44ch] text-lede">{body}</p>
      {onRetry && (
        <button type="button" onClick={onRetry} className={`${buttonFieldClass} mt-6`}>
          {retryLabel}
        </button>
      )}
    </div>
  );
}
