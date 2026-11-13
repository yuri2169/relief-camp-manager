import Link from "next/link";
import { Plus } from "@/components/icons";
import { buttonClass } from "@/components/ui";

export function EmptyState({
  title,
  body,
  action,
  headingLevel,
}: {
  title: string;
  body: string;
  action?: { label: string; href: string };
  headingLevel: 1 | 2 | 3;
}) {
  const Heading = `h${headingLevel}` as "h1" | "h2" | "h3";
  return (
    <div className="border-3 border-dashed border-ink px-5 py-10">
      <Heading className="text-title font-extrabold">{title}</Heading>
      <p className="mt-3 max-w-[44ch] text-lede">{body}</p>
      {action && (
        <Link href={action.href} className={`${buttonClass} mt-6`}>
          <Plus />
          {action.label}
        </Link>
      )}
    </div>
  );
}
