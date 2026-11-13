"use client";

import { useState } from "react";
import { buttonFieldClass } from "@/components/ui";
import { site } from "@/content/site";

export type TransferLine = {
  id: string;
  what: string;
  route: string;
};

export function TransferPanel({ lines }: { lines: TransferLine[] }) {
  const [applied, setApplied] = useState(false);
  const t = site.app.dashboard;

  return (
    <aside
      aria-labelledby="transfer-title"
      className="on-ink flex flex-col bg-ink px-6 py-7 text-paper"
    >
      <h2 id="transfer-title" className="text-2xl font-extrabold tracking-[-0.03em]">
        {t.transferTitle}
      </h2>
      {lines.length === 0 ? (
        <p className="mt-4 opacity-80">{t.transferEmpty}</p>
      ) : (
        <>
          <ul className="mt-3">
            {lines.map((line) => (
              <li key={line.id} className="border-b border-ink-rule py-3">
                <span className="block font-bold text-field">{line.what}</span>
                <span className="block text-small opacity-85">{line.route}</span>
              </li>
            ))}
          </ul>
          <button
            type="button"
            onClick={() => {
              if (!applied) setApplied(true);
            }}
            aria-disabled={applied}
            className={`${buttonFieldClass} mt-8 w-full text-lg lg:mt-auto ${applied ? "opacity-60" : ""}`}
          >
            {t.transferAction}
          </button>
          <p role="status" className="mt-3 min-h-10 text-sm">
            {applied ? t.transferDone : ""}
          </p>
        </>
      )}
    </aside>
  );
}
