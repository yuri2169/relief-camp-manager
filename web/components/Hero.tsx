import { Fragment } from "react";
import { site } from "@/content/site";
import { Sticker } from "./Sticker";

export function Hero() {
  const { headline, lede, sticker } = site.hero;
  return (
    <section className="relative bg-field px-5 pb-10 pt-4 md:px-8 md:pb-14 md:pt-6">
      <h1 className="text-poster font-extrabold">
        {headline.map((line) => (
          <Fragment key={line}>
            <span className="block">{line}</span>{" "}
          </Fragment>
        ))}
      </h1>
      <div className="mt-8 flex items-end justify-between gap-6 lg:mt-0">
        <p className="max-w-[30ch] text-lede lg:absolute lg:bottom-14 lg:right-8 lg:w-64">{lede}</p>
        <Sticker
          lines={sticker.lines}
          href={sticker.href}
          className="shrink-0 lg:absolute lg:right-14 lg:top-6"
        />
      </div>
    </section>
  );
}
