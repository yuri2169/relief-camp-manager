import type { Metadata } from "next";
import { EmptyState } from "@/components/app/EmptyState";
import { ErrorStateSample } from "@/components/app/ErrorStateSample";
import { LoadingState } from "@/components/app/LoadingState";
import { site } from "@/content/site";

export const metadata: Metadata = { title: site.app.states.title };

function Sample({ label, children }: { label: string; children: React.ReactNode }) {
  return (
    <section className="mt-10">
      <h2 className="border-t-3 border-ink pt-3 text-callout font-extrabold">{label}</h2>
      <div className="mt-5">{children}</div>
    </section>
  );
}

export default function StatesPage() {
  const t = site.app.states;
  const camps = site.app.camps;
  return (
    <div className="px-5 py-7 md:px-8">
      <h1 className="text-title font-extrabold">{t.title}</h1>
      <p className="mt-3 max-w-[56ch] text-lede">{t.intro}</p>

      <Sample label={t.empty}>
        <EmptyState
          title={camps.empty.title}
          body={camps.empty.body}
          action={{ label: site.app.register.title, href: "/app/register" }}
          headingLevel={3}
        />
      </Sample>
      <Sample label={t.loading}>
        <div className="border-3 border-ink">
          <LoadingState label={t.loadingLabel} />
        </div>
      </Sample>
      <Sample label={t.error}>
        <ErrorStateSample title={t.errorTitle} body={t.errorBody} retryLabel={t.retry} />
      </Sample>
    </div>
  );
}
