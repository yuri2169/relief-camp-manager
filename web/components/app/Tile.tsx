import Link from "next/link";
import { site } from "@/content/site";
import { oneDecimal, type CampSummary } from "@/lib/supply";

// One camp on the overview. A camp below the critical threshold takes the
// field colour and a text label, so the signal never relies on colour alone.
export function Tile({ summary }: { summary: CampSummary }) {
  const hot = summary.status === "low";
  const days = summary.days === null ? null : oneDecimal(summary.days);
  return (
    <Link
      href={`/app/camps/${summary.camp.id}`}
      className={`block h-full border-t-3 border-ink pb-5 pt-2.5 underline-offset-4 hover:underline ${hot ? "bg-field px-3" : ""}`}
    >
      <span className="block text-lg font-bold leading-tight">{summary.camp.name}</span>
      {days === null ? (
        <span className="text-sm font-bold">{site.app.notRationed}</span>
      ) : (
        <>
          <span className="text-figure font-extrabold tabular-nums">{days}</span>{" "}
          <span className="text-sm font-bold">{site.app.dashboard.daysUnit(days)}</span>
        </>
      )}
      {summary.criticalItem && (
        <span className="block text-sm font-bold">
          {site.app.detail.critical(summary.criticalItem)}
        </span>
      )}
    </Link>
  );
}
