import { Fragment } from "react";

// The dashboard's headline number: one giant figure on the field colour
// with a short label beside it.
export function BigCount({ count, lines }: { count: number; lines: string[] }) {
  return (
    <p className="flex items-end gap-4">
      <span className="bg-field px-4 pb-2.5 pt-3.5 text-mega font-extrabold tabular-nums">{count}</span>
      <span className="pb-1 text-callout font-extrabold">
        {lines.map((line) => (
          <Fragment key={line}>
            <span className="block">{line}</span>{" "}
          </Fragment>
        ))}
      </span>
    </p>
  );
}
