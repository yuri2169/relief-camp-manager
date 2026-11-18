"use client";

import { useRef, useState } from "react";
import { buttonClass } from "@/components/ui";
import { site } from "@/content/site";
import { CRITICAL_DAYS } from "@/lib/supply";
import { isValidThreshold } from "@/lib/validate-settings";
import { describedBy, Field, inputClass } from "./Field";

export function SettingsForm() {
  const t = site.app.settings;
  const f = t.fields;
  const [saved, setSaved] = useState(false);
  const [threshold, setThreshold] = useState(String(CRITICAL_DAYS));
  const [error, setError] = useState<string | undefined>();
  const thresholdRef = useRef<HTMLInputElement>(null);

  return (
    <form
      onSubmit={(event) => {
        event.preventDefault();
        if (!isValidThreshold(threshold)) {
          setError(t.errors.threshold);
          setSaved(false);
          thresholdRef.current?.focus();
          return;
        }
        setError(undefined);
        setSaved(true);
      }}
      onChange={() => setSaved(false)}
      noValidate
      className="space-y-6"
    >
      <Field id="district" label={f.district.label}>
        <input
          id="district"
          name="district"
          type="text"
          defaultValue={site.app.district}
          className={`${inputClass} max-w-md`}
        />
      </Field>

      <Field id="reportDay" label={f.reportDay.label} hint={f.reportDay.hint}>
        <select
          id="reportDay"
          name="reportDay"
          defaultValue={f.reportDay.options[0]}
          aria-describedby={describedBy("reportDay", f.reportDay.hint)}
          className={`${inputClass} max-w-60`}
        >
          {f.reportDay.options.map((day) => (
            <option key={day}>{day}</option>
          ))}
        </select>
      </Field>

      <Field id="threshold" label={f.threshold.label} hint={f.threshold.hint} error={error}>
        <input
          id="threshold"
          name="threshold"
          ref={thresholdRef}
          type="text"
          inputMode="numeric"
          value={threshold}
          onChange={(event) => setThreshold(event.target.value)}
          aria-invalid={error ? true : undefined}
          aria-describedby={describedBy("threshold", f.threshold.hint, error)}
          className={`${inputClass} max-w-40`}
        />
      </Field>

      <label className="flex items-center gap-3 text-lg font-bold">
        <input type="checkbox" name="autosave" defaultChecked className="size-6 accent-ink" />
        {f.autosave.label}
      </label>

      <div className="flex flex-wrap items-center gap-4">
        <button type="submit" className={`${buttonClass} text-lg`}>
          {t.submit}
        </button>
        <p role="status" className="font-bold">
          {saved ? t.saved : ""}
        </p>
      </div>
    </form>
  );
}
