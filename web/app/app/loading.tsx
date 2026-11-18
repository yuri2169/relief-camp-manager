import { LoadingState } from "@/components/app/LoadingState";
import { site } from "@/content/site";

export default function Loading() {
  return <LoadingState label={site.app.states.loadingLabel} />;
}
