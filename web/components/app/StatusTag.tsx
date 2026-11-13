import { site } from "@/content/site";
import type { Status } from "@/lib/supply";

const styles: Record<Status, string> = {
  low: "border-2 border-ink bg-ink text-field",
  watch: "border-2 border-ink bg-field text-ink",
  ok: "border-2 border-ink",
};

export function StatusTag({ status }: { status: Status }) {
  return (
    <span className={`inline-block whitespace-nowrap px-2 py-0.5 text-sm font-bold ${styles[status]}`}>
      {site.app.status[status]}
    </span>
  );
}
