import Image from "next/image";

// A framed slot for a screenshot or poster frame. With src=null it shows
// a labelled placeholder, so the layout is correct before you have media.
export function MediaSlot({
  src,
  alt,
  ratio,
  label,
}: {
  src: string | null;
  alt: string;
  ratio: string;
  label: string;
}) {
  return (
    <div className="relative border-3 border-ink bg-paper" style={{ aspectRatio: ratio }}>
      {src ? (
        <Image
          src={src}
          alt={alt}
          fill
          sizes="(min-width: 1024px) 70vw, 100vw"
          className="object-cover"
        />
      ) : (
        <div role="img" aria-label={label} className="hatch grid h-full place-items-center p-4 text-center">
          <span className="bg-paper px-3 py-1.5 text-sm font-bold">{label}</span>
        </div>
      )}
    </div>
  );
}
