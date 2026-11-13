import type { Metadata } from "next";
import { SettingsForm } from "@/components/app/SettingsForm";
import { site } from "@/content/site";

export const metadata: Metadata = { title: site.app.settings.title };

export default function SettingsPage() {
  return (
    <div className="px-5 py-7 md:px-8">
      <h1 className="text-title font-extrabold">{site.app.settings.title}</h1>
      <div className="mt-8">
        <SettingsForm />
      </div>
    </div>
  );
}
