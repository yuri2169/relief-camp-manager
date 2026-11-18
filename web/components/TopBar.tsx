import Link from "next/link";
import { site } from "@/content/site";
import { ArrowUpRight } from "./icons";
import { buttonSmallClass } from "./ui";

export function TopBar() {
  return (
    <header className="bg-field">
      <div className="flex h-15 items-center justify-between px-5 md:px-8">
        <Link href="/" className="text-xl font-extrabold tracking-[-0.03em]">
          {site.name}
        </Link>
        <nav aria-label={site.a11y.mainNav} className="flex items-center gap-5 text-small font-bold md:gap-7">
          {site.nav.map((item) => (
            <Link
              key={item.href}
              href={item.href}
              className="hidden underline-offset-4 hover:underline sm:inline"
            >
              {item.label}
            </Link>
          ))}
          <Link
            href={site.demo.cta.href}
            className={buttonSmallClass}
          >
            {site.demo.cta.short}
            <ArrowUpRight />
          </Link>
        </nav>
      </div>
    </header>
  );
}
