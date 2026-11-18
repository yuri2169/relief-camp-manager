import Link from "next/link";
import { Fragment } from "react";
import { ArrowRight } from "./icons";

// The one round shape in the starter. Use it once per page.
export function Sticker({
  lines,
  href,
  className = "",
}: {
  lines: string[];
  href: string;
  className?: string;
}) {
  return (
    <Link
      href={href}
      className={`grid size-28 rotate-[11deg] place-items-center rounded-full bg-ink text-center text-lg font-extrabold leading-none tracking-[-0.02em] text-field transition-transform hover:rotate-0 md:size-32 ${className}`}
    >
      <span>
        {lines.map((line) => (
          <Fragment key={line}>
            <span className="block">{line}</span>{" "}
          </Fragment>
        ))}
        <span className="mt-0.5 block">
          <ArrowRight className="inline" />
        </span>
      </span>
    </Link>
  );
}
