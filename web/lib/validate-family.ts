export type MemberDraft = {
  /** Stable key for React, so removing a row does not reshuffle state. */
  key: string;
  name: string;
  age: string;
  pregnant: boolean;
  disabled: boolean;
  lactating: boolean;
  duty: boolean;
};

export type FamilyDraft = {
  head: string;
  village: string;
  members: MemberDraft[];
};

export type FamilyErrors = {
  head?: string;
  village?: string;
  members?: string;
  /** Keyed by member key, then by field. */
  member: Record<string, { name?: string; age?: string }>;
};

const blank = { name: "", age: "", pregnant: false, disabled: false, lactating: false, duty: false };

/**
 * The first row's key is fixed, because it is rendered on the server as well
 * as on the client and the two must agree. Rows added after that are only ever
 * created in the browser, so a counter is safe for them.
 */
const FIRST_KEY = "m0";
let nextKey = 0;

export function emptyMember(): MemberDraft {
  nextKey += 1;
  return { key: `m${nextKey}`, ...blank };
}

export function emptyDraft(): FamilyDraft {
  return { head: "", village: "", members: [{ key: FIRST_KEY, ...blank }] };
}

const wholeNumber = /^\d+$/;

/** An age that is a whole number from 0 to 120, or null when it is not one yet. */
export function parseAge(value: string): number | null {
  const text = value.trim();
  if (!wholeNumber.test(text)) return null;
  const age = Number(text);
  return age >= 0 && age <= 120 ? age : null;
}

export function validateFamily(
  draft: FamilyDraft,
  messages: { required: string; age: string; noMembers: string },
): FamilyErrors {
  const errors: FamilyErrors = { member: {} };

  if (draft.head.trim() === "") errors.head = messages.required;
  if (draft.village.trim() === "") errors.village = messages.required;
  if (draft.members.length === 0) errors.members = messages.noMembers;

  for (const member of draft.members) {
    const found: { name?: string; age?: string } = {};
    if (member.name.trim() === "") found.name = messages.required;
    if (parseAge(member.age) === null) found.age = messages.age;
    if (found.name || found.age) errors.member[member.key] = found;
  }

  return errors;
}

export function hasErrors(errors: FamilyErrors): boolean {
  return Boolean(
    errors.head || errors.village || errors.members || Object.keys(errors.member).length > 0,
  );
}
