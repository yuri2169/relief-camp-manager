import type { Metadata } from "next";
import Link from "next/link";
import { FamilyForm } from "@/components/app/FamilyForm";
import { ArrowLeft } from "@/components/icons";
import { linkClass } from "@/components/ui";
import { site } from "@/content/site";

export const metadata: Metadata = { title: site.app.register.title };

export default function RegisterPage() {
  const t = site.app.register;
  return (
    <div className="px-5 py-7 md:px-8">
      <Link href="/app/camps" className={linkClass}>
        <ArrowLeft />
        {t.back}
      </Link>
      <h1 className="mt-5 text-title font-extrabold">{t.title}</h1>
      <p className="mt-3 max-w-[58ch] text-lede">{t.intro}</p>
      <div className="mt-8">
        <FamilyForm />
      </div>
    </div>
  );
}
