import { site } from "@/content/site";
import { ArrowUpRight } from "./icons";
import { linkClass } from "./ui";

export function Footer() {
  return (
    <footer className="on-ink bg-ink px-5 pb-8 pt-12 text-paper md:px-8">
      <p className="text-poster font-extrabold text-field">{site.name}</p>
      <div className="mt-8 flex flex-wrap justify-between gap-4 border-t border-ink-rule pt-5 text-small">
        <p className="max-w-[52ch] opacity-80">{site.footer.note}</p>
        <a href={site.repo.href} className={linkClass}>
          {site.repo.label}
          <ArrowUpRight />
        </a>
      </div>
    </footer>
  );
}
