# Poster starter: rules for extending it

This site looks like a conference poster: one loud flat colour, enormous
type, hard edges, no decoration. It is for hackathon projects that need to
be remembered after forty other demos. Keep it that blunt.

## The look

- **Colours:** `field` (the one accent, yellow by default), `ink` (black), `paper` (white), `ink-rule` (grey lines on black). Use the Tailwind utilities (`bg-field`, `text-ink`, `border-ink`). Never write a hex value in a component and never add a colour.
- **Type:** Bricolage Grotesque only. Weights are 500, 700 and 800 only. Display sizes come from tokens: `text-poster`, `text-mega`, `text-title`, `text-figure`, `text-callout`, `text-lede`, `text-small`. Tailwind's `text-sm`, `text-lg`, `text-xl` and `text-2xl` are fine for interface text. Headlines are `font-extrabold`. Body is weight 500.
- **Shape:** square corners everywhere. The round sticker is the single exception; use it once per page. Dividers are `border-t-3 border-ink`. No shadows.
- **Surfaces:** pages are white or field-coloured. Black is for bars, bands and panels only. A black container gets the `on-ink` class so focus rings inside it turn field-coloured. Never put `on-ink` on a button or link itself.
- **Emphasis** comes from size and from filling a block with `bg-field`, not from icons or colour variety.

## Signature elements

- The four-line headline.
- The round sticker (once per page).
- The black stat band.
- The giant count on a field block.
- Field-filled tiles for anything urgent.
- The black side panel.

## Never add

- Gradients, glow, blur, glass panels, drop shadows, rounded cards. The one exception is the `hatch` placeholder pattern in `styles/tokens.css`, a flat stripe made with a CSS gradient function.
- A switch of the build to webpack. Tailwind is wired through the Turbopack loader.
- Emoji, or an icon library. New icons go in `components/icons.tsx`, drawn in the same square-capped style.
- A badge pill above a headline, a three-across grid of icon cards, testimonials, logo walls, star ratings.
- A second accent colour or a second typeface.
- A dark page background.
- shadcn/ui, Radix, Lucide, Framer Motion, or any other UI or animation dependency.

## Words

All copy lives in `content/site.ts`. Components contain no strings. Write the
way the demo copy does: short, concrete, with real numbers and where they
came from. No "supercharge", "seamless", "revolutionary" or "unlock". No
exclamation marks.

## Where things live

- `content/site.ts`: every string. `content/demo-data.ts`: sample records.
- `styles/tokens.css`: colours, type sizes, animation.
- `components/`: landing sections. `components/app/`: app pieces.
- `lib/`: pure functions (forecast maths, form validation).

## Adding a page

1. Add its copy to `content/site.ts`.
2. Create `app/<route>/page.tsx` and compose it from existing components.
3. Export `metadata` with a `title` taken from `site`.
4. Give it exactly one `h1`, using `text-title font-extrabold`.
5. For a form, use `Field`, `inputClass` and `describedBy` from `components/app/Field.tsx`, and give every field its own width class.
6. Use `buttonClass`, `buttonSmallClass`, `buttonFieldClass` and `linkClass` from `components/ui.ts` instead of writing button classes.
7. Give `EmptyState` and `ErrorState` a `headingLevel` that fits the page.
8. Check it at 375px wide and with the keyboard before calling it done.

@AGENTS.md
