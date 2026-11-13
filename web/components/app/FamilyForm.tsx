"use client";

import { useEffect, useRef, useState } from "react";
import { buttonClass, buttonSmallClass } from "@/components/ui";
import { Plus } from "@/components/icons";
import { itemNames } from "@/content/demo-data";
import { site } from "@/content/site";
import { amount, totalsFor, typeOf, canTakeDuty, type Member } from "@/lib/ration";
import {
  emptyDraft,
  emptyMember,
  hasErrors,
  parseAge,
  validateFamily,
  type FamilyDraft,
  type FamilyErrors,
  type MemberDraft,
} from "@/lib/validate-family";
import { describedBy, Field, inputClass } from "./Field";

const noErrors: FamilyErrors = { member: {} };

/** Only rows with a usable age can be costed, so the card fills in as you type. */
function readable(members: MemberDraft[]): Member[] {
  return members
    .map((member) => {
      const age = parseAge(member.age);
      if (age === null) return null;
      return {
        name: member.name.trim(),
        age,
        pregnant: member.pregnant,
        disabled: member.disabled,
        lactating: member.lactating,
        duty: member.duty,
      };
    })
    .filter((member): member is Member => member !== null);
}

export function FamilyForm() {
  const t = site.app.register;
  const [draft, setDraft] = useState<FamilyDraft>(emptyDraft);
  const [errors, setErrors] = useState<FamilyErrors>(noErrors);
  const [saved, setSaved] = useState<{ head: string; score: number } | null>(null);
  const summaryRef = useRef<HTMLDivElement>(null);
  const panelRef = useRef<HTMLDivElement>(null);
  const focusHead = useRef(false);

  useEffect(() => {
    if (saved) {
      panelRef.current?.focus();
    } else if (focusHead.current) {
      focusHead.current = false;
      document.getElementById("head")?.focus();
    }
  }, [saved]);

  const costed = readable(draft.members);
  const totals = totalsFor(costed);

  function setMember(key: string, patch: Partial<MemberDraft>) {
    setDraft((current) => ({
      ...current,
      members: current.members.map((member) =>
        member.key === key ? { ...member, ...patch } : member,
      ),
    }));
  }

  function onSubmit(event: React.FormEvent) {
    event.preventDefault();
    const found = validateFamily(draft, t.errors);
    setErrors(found);
    if (hasErrors(found)) {
      requestAnimationFrame(() => summaryRef.current?.focus());
      return;
    }
    setSaved({ head: draft.head.trim(), score: totals.score });
  }

  const status = (
    <p role="status" className="sr-only">
      {saved ? [t.saved.title, saved.head, t.saved.body].join(" ") : ""}
    </p>
  );

  if (saved) {
    return (
      <>
        {status}
        <div ref={panelRef} tabIndex={-1} className="max-w-xl bg-field p-6">
          <p className="text-title font-extrabold">{t.saved.title}</p>
          <p className="mt-3 text-lede font-bold">{saved.head}</p>
          <p className="text-lede">{t.saved.body}</p>
          <p className="mt-4 text-figure font-extrabold tabular-nums">{saved.score}</p>
          <p className="font-bold">{t.card.score}</p>
          <button
            type="button"
            onClick={() => {
              focusHead.current = true;
              setSaved(null);
              setDraft(emptyDraft());
              setErrors(noErrors);
            }}
            className={`${buttonClass} mt-6`}
          >
            {t.saved.again}
          </button>
        </div>
      </>
    );
  }

  const summaryLines: { href: string; text: string }[] = [];
  if (errors.head) summaryLines.push({ href: "#head", text: t.errorLine(t.fields.head.label, errors.head) });
  if (errors.village)
    summaryLines.push({ href: "#village", text: t.errorLine(t.fields.village.label, errors.village) });
  draft.members.forEach((member, index) => {
    const found = errors.member[member.key];
    if (found?.name)
      summaryLines.push({
        href: `#${member.key}-name`,
        text: t.errorLine(`${t.members.position(index + 1)}, ${t.members.name}`, found.name),
      });
    if (found?.age)
      summaryLines.push({
        href: `#${member.key}-age`,
        text: t.errorLine(`${t.members.position(index + 1)}, ${t.members.age}`, found.age),
      });
  });

  return (
    <>
      {status}
      <div className="grid gap-10 lg:grid-cols-[1fr_20rem] lg:gap-12">
        <form onSubmit={onSubmit} noValidate className="space-y-6">
          {summaryLines.length > 0 && (
            <div ref={summaryRef} tabIndex={-1} className="max-w-md border-3 border-ink p-4">
              <h2 className="text-xl font-extrabold">{t.errorSummary}</h2>
              <ul className="mt-2 space-y-1">
                {summaryLines.map((line) => (
                  <li key={line.href}>
                    <a href={line.href} className="font-bold underline underline-offset-4">
                      {line.text}
                    </a>
                  </li>
                ))}
              </ul>
            </div>
          )}

          <Field id="head" label={t.fields.head.label} hint={t.fields.head.hint} error={errors.head}>
            <input
              id="head"
              name="head"
              type="text"
              value={draft.head}
              onChange={(event) => setDraft({ ...draft, head: event.target.value })}
              aria-invalid={errors.head ? true : undefined}
              aria-describedby={describedBy("head", t.fields.head.hint, errors.head)}
              className={`${inputClass} max-w-md`}
            />
          </Field>

          <Field
            id="village"
            label={t.fields.village.label}
            hint={t.fields.village.hint}
            error={errors.village}
          >
            <input
              id="village"
              name="village"
              type="text"
              value={draft.village}
              onChange={(event) => setDraft({ ...draft, village: event.target.value })}
              aria-invalid={errors.village ? true : undefined}
              aria-describedby={describedBy("village", t.fields.village.hint, errors.village)}
              className={`${inputClass} max-w-md`}
            />
          </Field>

          <fieldset className="border-t-3 border-ink pt-4">
            <legend className="sr-only">{t.members.title}</legend>
            <h2 className="text-callout font-extrabold">{t.members.title}</h2>
            <p className="mt-1 max-w-[58ch] text-small opacity-75">{t.members.hint}</p>
            {errors.members && (
              <p className="mt-3 border-l-5 border-ink pl-2 font-bold">{errors.members}</p>
            )}

            <ol className="mt-5 space-y-7">
              {draft.members.map((member, index) => {
                const found = errors.member[member.key];
                const age = parseAge(member.age);
                const shown =
                  age === null
                    ? null
                    : t.types[typeOf({ ...member, age, name: member.name })];
                return (
                  <li key={member.key} className="border-t border-ink/20 pt-5">
                    <div className="flex flex-wrap items-baseline justify-between gap-x-4 gap-y-1">
                      <h3 className="text-lg font-extrabold">
                        {t.members.position(index + 1)}
                        {shown && <span className="ml-3 font-bold opacity-70">{shown}</span>}
                      </h3>
                      {draft.members.length > 1 && (
                        <button
                          type="button"
                          onClick={() =>
                            setDraft({
                              ...draft,
                              members: draft.members.filter((other) => other.key !== member.key),
                            })
                          }
                          aria-label={t.members.removeLabel(index + 1)}
                          className="cursor-pointer font-bold underline decoration-3 underline-offset-4"
                        >
                          {t.members.remove}
                        </button>
                      )}
                    </div>

                    <div className="mt-3 flex flex-wrap gap-x-5 gap-y-4">
                      <Field
                        id={`${member.key}-name`}
                        label={t.members.name}
                        error={found?.name}
                      >
                        <input
                          id={`${member.key}-name`}
                          type="text"
                          value={member.name}
                          onChange={(event) => setMember(member.key, { name: event.target.value })}
                          aria-invalid={found?.name ? true : undefined}
                          aria-describedby={describedBy(`${member.key}-name`, undefined, found?.name)}
                          className={`${inputClass} w-56`}
                        />
                      </Field>
                      <Field id={`${member.key}-age`} label={t.members.age} error={found?.age}>
                        <input
                          id={`${member.key}-age`}
                          type="text"
                          inputMode="numeric"
                          value={member.age}
                          onChange={(event) => setMember(member.key, { age: event.target.value })}
                          aria-invalid={found?.age ? true : undefined}
                          aria-describedby={describedBy(`${member.key}-age`, undefined, found?.age)}
                          className={`${inputClass} w-24`}
                        />
                      </Field>
                    </div>

                    <div className="mt-4 flex flex-wrap gap-x-6 gap-y-3">
                      {(["pregnant", "disabled", "lactating"] as const).map((flag) => (
                        <label key={flag} className="flex items-center gap-2.5 font-bold">
                          <input
                            type="checkbox"
                            checked={member[flag]}
                            onChange={(event) => setMember(member.key, { [flag]: event.target.checked })}
                            className="size-5 accent-ink"
                          />
                          {t.members[flag]}
                        </label>
                      ))}
                      {age !== null && canTakeDuty(age) && (
                        <label className="flex items-center gap-2.5 font-bold">
                          <input
                            type="checkbox"
                            checked={member.duty}
                            onChange={(event) => setMember(member.key, { duty: event.target.checked })}
                            className="size-5 accent-ink"
                          />
                          {t.members.duty}
                        </label>
                      )}
                    </div>
                  </li>
                );
              })}
            </ol>

            <button
              type="button"
              onClick={() => setDraft({ ...draft, members: [...draft.members, emptyMember()] })}
              className={`${buttonSmallClass} mt-6`}
            >
              <Plus />
              {t.members.add}
            </button>
          </fieldset>

          <button type="submit" className={`${buttonClass} text-lg`}>
            {t.submit}
          </button>
        </form>

        <aside aria-labelledby="card-title" className="self-start bg-field p-5 lg:sticky lg:top-5">
          <h2 id="card-title" className="text-callout font-extrabold">
            {t.card.title}
          </h2>
          {costed.length === 0 ? (
            <p className="mt-3">{t.card.empty}</p>
          ) : (
            <>
              <p className="mt-4 text-mega font-extrabold leading-none tabular-nums">
                {totals.score}
              </p>
              <p className="font-bold">{t.card.score}</p>

              <h3 className="mt-6 border-t-3 border-ink pt-3 font-extrabold">{t.card.members}</h3>
              <p>
                {t.card.composition(
                  (Object.entries(totals.counts) as [keyof typeof totals.counts, number][])
                    .filter(([, count]) => count > 0)
                    .map(([category, count]) => `${count} ${t.types[category].toLowerCase()}`),
                )}
              </p>

              <h3 className="mt-5 border-t-3 border-ink pt-3 font-extrabold">{t.card.daily}</h3>
              <dl>
                {totals.ration.map((line) => (
                  <div key={line.item} className="flex justify-between gap-4 py-0.5">
                    <dt>{itemNames[line.item].name}</dt>
                    <dd className="tabular-nums">
                      {site.app.qty(amount(line.amount), itemNames[line.item].unit)}
                    </dd>
                  </div>
                ))}
              </dl>

              <p className="mt-5 border-t border-ink/30 pt-3 text-small">{t.card.scoreNote}</p>
              {costed.some((member) => typeOf(member) === "resident-volunteer") && (
                <p className="mt-2 text-small">{t.card.dutyNote}</p>
              )}
            </>
          )}
        </aside>
      </div>
    </>
  );
}
