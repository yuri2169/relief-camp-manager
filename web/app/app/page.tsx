import type { Metadata } from "next";
import Link from "next/link";
import { BigCount } from "@/components/app/BigCount";
import { Tile } from "@/components/app/Tile";
import { TransferPanel } from "@/components/app/TransferPanel";
import { ArrowRight } from "@/components/icons";
import { linkClass } from "@/components/ui";
import { site } from "@/content/site";
import { allSummaries, oneDecimal, transfers } from "@/lib/supply";

export const metadata: Metadata = { title: site.app.dashboard.title };

export default function Overview() {
  const t = site.app.dashboard;
  const summaries = allSummaries();
  const moves = transfers();
  const lowCount = summaries.reduce((total, summary) => total + summary.lowItems.length, 0);

  return (
    <div className="grid flex-1 lg:grid-cols-[1fr_20rem]">
      <div className="px-5 py-7 md:px-8">
        <h1 className="sr-only">{t.title}</h1>
        <BigCount count={lowCount} lines={t.countLabel(lowCount)} />
        <ul className="mt-7 grid grid-cols-2 gap-x-5 md:grid-cols-3">
          {summaries.map((summary) => (
            <li key={summary.camp.id}>
              <Tile summary={summary} />
            </li>
          ))}
        </ul>
        <Link href="/app/camps" className={`${linkClass} mt-2`}>
          {t.allCamps}
          <ArrowRight />
        </Link>
      </div>
      <TransferPanel
        lines={moves.map((move) => ({
          id: move.id,
          what: t.transferLine(oneDecimal(move.quantity), move.unit, move.itemName),
          route: t.transferRoute(move.from.name, move.to.name),
        }))}
      />
    </div>
  );
}
