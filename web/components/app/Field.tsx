// Label, hint and error for one form control. Pass the control as children
// and give it id={id} and aria-describedby={describedBy(id, hint, error)}.
export function Field({
  id,
  label,
  hint,
  error,
  children,
}: {
  id: string;
  label: string;
  hint?: string;
  error?: string;
  children: React.ReactNode;
}) {
  return (
    <div>
      <label htmlFor={id} className="block text-lg font-bold">
        {label}
      </label>
      {hint && (
        <p id={`${id}-hint`} className="text-small opacity-75">
          {hint}
        </p>
      )}
      {error && (
        <p id={`${id}-error`} className="mt-1 border-l-5 border-ink pl-2 font-bold">
          {error}
        </p>
      )}
      <div className="mt-2">{children}</div>
    </div>
  );
}

export const inputClass =
  "block w-full border-3 border-ink bg-paper px-3 py-2.5 text-lg font-medium";

export function describedBy(id: string, hint?: string, error?: string): string | undefined {
  const ids = [hint && `${id}-hint`, error && `${id}-error`].filter(Boolean);
  return ids.length > 0 ? ids.join(" ") : undefined;
}
