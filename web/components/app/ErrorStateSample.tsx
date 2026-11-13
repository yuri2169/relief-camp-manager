"use client";

import { useRouter } from "next/navigation";
import { ErrorState } from "./ErrorState";

// The gallery's error sample: a real button, so the retry style can be seen working.
export function ErrorStateSample({
  title,
  body,
  retryLabel,
}: {
  title: string;
  body: string;
  retryLabel: string;
}) {
  const router = useRouter();
  return (
    <ErrorState
      title={title}
      body={body}
      retryLabel={retryLabel}
      onRetry={() => router.refresh()}
      headingLevel={3}
    />
  );
}
