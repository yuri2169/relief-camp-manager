"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import { site } from "@/content/site";

export function AppBar() {
  const path = usePathname();
  const isActive = (href: string) => (href === "/app" ? path === "/app" : path.startsWith(href));

  return (
    <header className="on-ink bg-ink text-paper">
      {/* Four nav items do not fit beside the wordmark at 375px, so the bar
          stacks into two rows until there is room for one. */}
      <div className="flex flex-col gap-1.5 px-5 py-3 md:h-14 md:flex-row md:items-center md:justify-between md:gap-4 md:px-8 md:py-0">
        <div className="flex items-baseline justify-between gap-4">
          <Link
            href="/"
            className="whitespace-nowrap text-xl font-extrabold tracking-[-0.03em] text-field"
          >
            {site.name}
          </Link>
          <span className="text-small opacity-75 md:hidden">{site.app.district}</span>
        </div>
        <nav
          aria-label={site.a11y.appNav}
          className="flex gap-5 text-small font-bold md:gap-7"
        >
          {site.app.nav.map((item) => (
            <Link
              key={item.href}
              href={item.href}
              aria-current={isActive(item.href) ? "page" : undefined}
              className="opacity-65 decoration-field decoration-3 underline-offset-[6px] hover:opacity-100 aria-[current=page]:underline aria-[current=page]:opacity-100"
            >
              {item.label}
            </Link>
          ))}
        </nav>
        <span className="hidden text-small opacity-75 md:inline">{site.app.district}</span>
      </div>
    </header>
  );
}
