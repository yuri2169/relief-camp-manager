import Link from "next/link";
import { site } from "@/content/site";
import { ArrowRight } from "./icons";
import { buttonClass } from "./ui";
import { MediaSlot } from "./MediaSlot";
import { Section } from "./Section";

export function Demo() {
  const { title, body, cta, media } = site.demo;
  return (
    <Section id="demo" title={title} tone="field">
      <div className="grid gap-8 lg:grid-cols-[1fr_18rem] lg:items-end">
        <MediaSlot src={media.src} alt={media.alt} ratio="16 / 10" label={media.placeholder} />
        <div>
          <p className="text-lede">{body}</p>
          <Link
            href={cta.href}
            className={`${buttonClass} mt-5 text-lg`}
          >
            {cta.label}
            <ArrowRight />
          </Link>
        </div>
      </div>
    </Section>
  );
}
