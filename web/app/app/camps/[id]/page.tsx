import type { Metadata } from "next";
import Link from "next/link";
import { notFound } from "next/navigation";
import { Suspense } from "react";
import { LoadingState } from "@/components/app/LoadingState";
import { StatusTag } from "@/components/app/StatusTag";
import { SupplyBars } from "@/components/app/SupplyBars";
import { ArrowLeft } from "@/components/icons";
import { linkClass } from "@/components/ui";
import { camps, findCamp } from "@/content/demo-data";
import { site } from "@/content/site";
import { oneDecimal, stockRows, summarise } from "@/lib/supply";

export function generateStaticParams() {
  return camps.map((camp) => ({ id: camp.id }));
}

export async function generateMetadata({
  params,
}: {
  params: Promise<{ id: string }>;
}): Promise<Metadata> {
  const { id } = await params;
  const camp = findCamp(id);
  return camp ? { title: camp.name } : {};
}

async function CampDetail({ params }: { params: Promise<{ id: string }> }) {
  const { id } = await params;
  const camp = findCamp(id);
  if (!camp) notFound();

  const t = site.app.detail;
  const summary = summarise(camp);
  const rows = stockRows(camp);
  const rationed = rows.filter((row) => row.days !== null);
  const bars = rationed.map((row) => ({ name: row.name, days: row.days as number }));

  const facts: [string, string][] = [
    [t.facts.number, String(camp.number)],
    [t.facts.district, camp.district],
    [t.facts.occupancy, site.app.camps.occupancy(camp.occupancy, camp.capacity)],
    [t.facts.free, String(summary.free)],
    [t.facts.shelters, String(camp.shelters)],
  ];

  return (
    <div className="px-5 py-7 md:px-8">
      <Link href="/app/camps" className={linkClass}>
        <ArrowLeft />
        {t.back}
      </Link>

      <div className="mt-5 flex flex-wrap items-center gap-x-5 gap-y-2">
        <h1 className="text-title font-extrabold">{camp.name}</h1>
        <StatusTag status={summary.status} />
      </div>

      <div className="mt-8 grid gap-10 lg:grid-cols-[auto_1fr] lg:gap-16">
        {summary.days === null ? (
          <p className="self-start text-callout font-extrabold">{site.app.notRationed}</p>
        ) : (
          <div className="self-start">
            <p className="flex items-end gap-4">
              <span
                className={`px-4 pb-2.5 pt-3.5 text-mega font-extrabold tabular-nums ${summary.status === "low" ? "bg-field" : "border-3 border-ink"}`}
              >
                {oneDecimal(summary.days)}
              </span>
              <span className="max-w-[11ch] pb-1 text-callout font-extrabold">{t.daysLeft}</span>
            </p>
            {summary.criticalItem && (
              <p className="mt-3 font-bold">{t.critical(summary.criticalItem)}</p>
            )}
          </div>
        )}

        <div>
          <dl className="border-t-3 border-ink">
            {facts.map(([label, value]) => (
              <div key={label} className="flex justify-between gap-6 border-b border-ink/20 py-3">
                <dt className="font-bold">{label}</dt>
                <dd className="text-right tabular-nums">{value}</dd>
              </div>
            ))}
          </dl>
        </div>
      </div>

      <section aria-labelledby="stock-title" className="mt-14">
        <h2 id="stock-title" className="text-callout font-extrabold">
          {t.stockTitle}
        </h2>
        <table className="mt-4 w-full border-collapse text-left">
          <thead>
            <tr className="border-b-3 border-ink text-sm">
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.item}</th>
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.onHand}</th>
              <th scope="col" className="hidden py-2 pr-3 font-bold sm:table-cell">
                {t.columns.perDay}
              </th>
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.supply}</th>
              <th scope="col" className="py-2 font-bold">{t.columns.status}</th>
            </tr>
          </thead>
          <tbody>
            {rows.map((row) => (
              <tr key={row.item} className="border-b border-ink/20">
                <th scope="row" className="py-3 pr-3 text-lg font-bold">{row.name}</th>
                <td className="py-3 pr-3 tabular-nums">
                  {site.app.qty(oneDecimal(row.quantity), row.unit)}
                </td>
                <td className="hidden py-3 pr-3 tabular-nums sm:table-cell">
                  {row.days === null ? site.app.dash : oneDecimal(row.dailyDemand)}
                </td>
                <td className="py-3 pr-3 tabular-nums">
                  {row.days === null ? site.app.notRationed : oneDecimal(row.days)}
                </td>
                <td className="py-3">
                  {row.days === null ? site.app.dash : <StatusTag status={row.status} />}
                </td>
              </tr>
            ))}
          </tbody>
        </table>

        <div className="mt-12 max-w-2xl">
          <SupplyBars
            bars={bars}
            caption={t.chartCaption}
            label={t.chartLabel(bars.map((bar) => `${bar.name} ${oneDecimal(bar.days)}`))}
          />
        </div>
      </section>

      <section aria-labelledby="health-title" className="mt-14">
        <h2 id="health-title" className="text-callout font-extrabold">
          {t.healthTitle}
        </h2>
        <dl className="mt-4 border-t-3 border-ink">
          {camp.health.map((line) => (
            <div
              key={line.symptom}
              className={`flex flex-wrap justify-between gap-x-6 border-b border-ink/20 py-3 ${line.cases >= line.threshold ? "bg-field px-3" : ""}`}
            >
              <dt className="text-lg font-bold">{line.symptom}</dt>
              <dd>{t.healthLine(line.cases, line.threshold)}</dd>
            </div>
          ))}
        </dl>
      </section>
    </div>
  );
}

export default function CampPage({ params }: { params: Promise<{ id: string }> }) {
  return (
    <Suspense fallback={<LoadingState label={site.app.states.loadingLabel} />}>
      <CampDetail params={params} />
    </Suspense>
  );
}
