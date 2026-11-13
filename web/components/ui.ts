// Shared classes for every button-like control and inline link, so the look
// is defined once. Append per-use extras (margins, w-full, text-lg) after them.
// Never add the on-ink class to a button or link: it belongs on the container.

const press = "cursor-pointer hover:underline underline-offset-4 decoration-3 active:translate-y-px";

/** Primary button: ink fill, field-coloured text. */
export const buttonClass = `inline-flex items-center justify-center gap-2 bg-ink px-5 py-3.5 font-extrabold text-field ${press}`;

/** The same button, smaller, for bars. */
export const buttonSmallClass = `inline-flex items-center justify-center gap-2 bg-ink px-4 py-2.5 font-bold text-field ${press}`;

/** Field fill, ink text, for buttons inside black (on-ink) containers. */
export const buttonFieldClass = `inline-flex items-center justify-center gap-2 bg-field px-5 py-3.5 font-extrabold text-ink ${press}`;

/** Back links and inline links. */
export const linkClass = "inline-flex items-center gap-2 font-bold underline underline-offset-4";
