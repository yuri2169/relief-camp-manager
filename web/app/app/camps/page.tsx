import type { Metadata } from "next";
import Link from "next/link";
import { EmptyState } from "@/components/app/EmptyState";
import { StatusTag } from "@/components/app/StatusTag";
import { site } from "@/content/site";
import { allSummaries, oneDecimal } from "@/lib/supply";

export const metadata: Metadata = { title: site.app.camps.title };

export default function CampsPage() {
  const t = site.app.camps;
  const summaries = allSummaries();

  return (
    <div className="px-5 py-7 md:px-8">
      <h1 className="text-title font-extrabold">{t.title}</h1>

      {summaries.length === 0 ? (
        <div className="mt-8">
          <EmptyState
            title={t.empty.title}
            body={t.empty.body}
            action={{ label: site.app.register.title, href: "/app/register" }}
            headingLevel={2}
          />
        </div>
      ) : (
        <table className="mt-8 w-full border-collapse text-left">
          <thead>
            <tr className="border-b-3 border-ink text-sm">
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.name}</th>
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.occupancy}</th>
              <th scope="col" className="hidden py-2 pr-3 font-bold sm:table-cell">
                {t.columns.shelters}
              </th>
              <th scope="col" className="py-2 pr-3 font-bold">{t.columns.supply}</th>
              <th scope="col" className="py-2 font-bold">{t.columns.status}</th>
            </tr>
          </thead>
          <tbody>
            {summaries.map((summary) => (
              <tr key={summary.camp.id} className="border-b border-ink/20">
                <th scope="row" className="py-3.5 pr-3 text-lg font-bold">
                  <Link
                    href={`/app/camps/${summary.camp.id}`}
                    className="underline underline-offset-4"
                  >
                    {summary.camp.name}
                  </Link>
                </th>
                <td className="py-3.5 pr-3 tabular-nums">
                  {t.occupancy(summary.camp.occupancy, summary.camp.capacity)}
                  {summary.free === 0 && <span className="font-bold"> {t.full}</span>}
                </td>
                <td className="hidden py-3.5 pr-3 tabular-nums sm:table-cell">
                  {summary.camp.shelters}
                </td>
                <td className="py-3.5 pr-3 tabular-nums">
                  {summary.days === null ? site.app.dash : oneDecimal(summary.days)}
                </td>
                <td className="py-3.5">
                  <StatusTag status={summary.status} />
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </div>
  );
}
