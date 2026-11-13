import type { Metadata } from "next";
import Link from "next/link";
import { site } from "@/content/site";
import { ArrowLeft } from "@/components/icons";
import { buttonClass } from "@/components/ui";

export const metadata: Metadata = { title: site.notFound.title };

export default function NotFound() {
  const t = site.notFound;
  return (
    <main id="main" className="flex min-h-dvh flex-col justify-between bg-field px-5 py-8 md:px-8">
      <p className="text-mega font-extrabold">{t.figure}</p>
      <div>
        <h1 className="text-title font-extrabold">{t.title}</h1>
        <p className="mt-3 text-lede">{t.body}</p>
        <Link
          href={t.action.href}
          className={`${buttonClass} mt-6 text-lg`}
        >
          <ArrowLeft />
          {t.action.label}
        </Link>
      </div>
    </main>
  );
}
