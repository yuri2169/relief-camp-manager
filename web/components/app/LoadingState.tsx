export function LoadingState({ label }: { label: string }) {
  return (
    <div role="status" className="flex items-end gap-4 px-5 py-10 md:px-8">
      <div aria-hidden className="flex h-16 items-end gap-1.5">
        {[0, 1, 2, 3].map((bar) => (
          <span
            key={bar}
            className="block h-full w-4 origin-bottom animate-pulse-bar bg-ink"
            style={{ animationDelay: `${bar * 0.14}s` }}
          />
        ))}
      </div>
      <p className="text-callout font-extrabold">{label}</p>
    </div>
  );
}
