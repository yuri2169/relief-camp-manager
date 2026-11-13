import { Demo } from "@/components/Demo";
import { Footer } from "@/components/Footer";
import { Hero } from "@/components/Hero";
import { HowItWorks } from "@/components/HowItWorks";
import { Inside } from "@/components/Inside";
import { Problem } from "@/components/Problem";
import { StatBand } from "@/components/StatBand";
import { TopBar } from "@/components/TopBar";

export default function Home() {
  return (
    <>
      <TopBar />
      <main id="main">
        <Hero />
        <StatBand />
        <Problem />
        <HowItWorks />
        <Demo />
        <Inside />
      </main>
      <Footer />
    </>
  );
}
