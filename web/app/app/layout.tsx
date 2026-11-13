import { AppBar } from "@/components/app/AppBar";

export default function AppLayout({ children }: { children: React.ReactNode }) {
  return (
    <div className="flex min-h-dvh flex-col">
      <AppBar />
      <main id="main" className="flex flex-1 flex-col">
        {children}
      </main>
    </div>
  );
}
