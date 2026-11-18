"use client";

import { ErrorState } from "@/components/app/ErrorState";
import { site } from "@/content/site";

export default function AppError({ retry }: { error: Error & { digest?: string }; retry: () => void }) {
  const t = site.app.states;
  return <ErrorState title={t.errorTitle} body={t.errorBody} retryLabel={t.retry} onRetry={retry} announce headingLevel={1} />;
}
