// The only icons in this starter. Drawn square-capped to match the type.
// Add new ones here in the same style; do not install an icon library.
type IconProps = { className?: string };

const base = {
  width: 20,
  height: 20,
  viewBox: "0 0 20 20",
  fill: "none",
  stroke: "currentColor",
  strokeWidth: 2.5,
  strokeLinecap: "square" as const,
  "aria-hidden": true,
};

export function ArrowUpRight({ className }: IconProps) {
  return (
    <svg {...base} className={className}>
      <path d="M5 15 15 5M7 5h8v8" />
    </svg>
  );
}

export function ArrowRight({ className }: IconProps) {
  return (
    <svg {...base} className={className}>
      <path d="M3 10h14M11 4l6 6-6 6" />
    </svg>
  );
}

export function ArrowLeft({ className }: IconProps) {
  return (
    <svg {...base} className={className}>
      <path d="M17 10H3M9 4l-6 6 6 6" />
    </svg>
  );
}

export function Plus({ className }: IconProps) {
  return (
    <svg {...base} className={className}>
      <path d="M10 3v14M3 10h14" />
    </svg>
  );
}
