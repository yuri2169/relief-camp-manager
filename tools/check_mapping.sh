#!/bin/sh
# Usage (from the repository root): sh tools/check_mapping.sh
# Compares the tags in the source with the tag index at the end of
# docs/OOP_MAPPING.md (case-insensitive, singular/plural folded), checks that
# every file:line cited in the document lies inside its file, and that every
# syllabus concept has at least one tag. Exits non-zero on any divergence.
DOC=docs/OOP_MAPPING.md
TMP=${TMPDIR:-/tmp}/relief_map.$$
trap 'rm -f "$TMP".*' EXIT
STATUS=0

# normalise: "file:line: [Tag] ..." -> "file:line tag" (lowercase, trailing s dropped)
norm() {
  sed -n 's/^\([^:]*:[0-9]*\): \[\([^]]*\)\].*/\1 \2/p' |
    awk '{ k = index($0, " "); p = substr($0, 1, k - 1); t = tolower(substr($0, k + 1));
           sub(/s$/, "", t); print p " " t }' | sort -u
}

sh tools/collect_tags.sh | norm > "$TMP.src"
# the index sits between the two marker lines in the document
sed -n '/^<!-- tag-index-begin -->$/,/^<!-- tag-index-end -->$/p' "$DOC" |
  grep '^[a-z]*/[A-Za-z]*\.[a-z]*:[0-9]*: \[' | norm > "$TMP.doc"

if [ ! -s "$TMP.doc" ]; then echo "FAIL: no tag index found in $DOC"; exit 1; fi

ONLY_SRC=$(comm -23 "$TMP.src" "$TMP.doc")
ONLY_DOC=$(comm -13 "$TMP.src" "$TMP.doc")
if [ -n "$ONLY_SRC" ]; then echo "In source but not in $DOC index:"; echo "$ONLY_SRC"; STATUS=1; fi
if [ -n "$ONLY_DOC" ]; then echo "In $DOC index but not in source:"; echo "$ONLY_DOC"; STATUS=1; fi

# every cited include/... or src/...:N must exist in its file
grep -o '\(include\|src\)/[A-Za-z]*\.[a-z]*:[0-9][0-9,-]*' "$DOC" | sort -u | while read -r ref; do
  file=${ref%%:*}; nums=${ref#*:}
  len=$(wc -l < "$file" 2>/dev/null | tr -d ' ')
  if [ -z "$len" ]; then echo "Cited file missing: $file"; echo bad > "$TMP.bad"; continue; fi
  for n in $(echo "$nums" | tr ',-' '  '); do
    if [ "$n" -gt "$len" ]; then echo "Cited line past end of file: $file:$n (file has $len)"; echo bad > "$TMP.bad"; fi
  done
done
[ -f "$TMP.bad" ] && STATUS=1

# every syllabus concept needs a tag
for kw in "static data member" "static member function" "friend function" "friend class" \
  "nested class" "pure virtual" "virtual base class" "virtual destructor" "copy constructor" \
  "copy assignment" "destructor" "dynamic memory" "array of dynamic objects" \
  "pointer to member function" "class template" "function template" "basic type to class type" \
  "class to basic type conversion" "class to class conversion" "single inheritance" \
  "multilevel inheritance" "hierarchical inheritance" "multiple inheritance" "hybrid inheritance" \
  "unary operator" "binary operator" "as argument" "returning object" "const member function" \
  "constant object" "custom exception base class" "derived exception" "exception handling" \
  "file handling" "multithreading" "abstract class" "runtime polymorphism" "object lifecycle"; do
  folded=$(echo "$kw" | sed 's/s$//')
  if ! grep -q -F "$folded" "$TMP.src"; then echo "MISSING TAG for concept: $kw"; STATUS=1; fi
done

if [ "$STATUS" -eq 0 ]; then
  echo "PASS: $(wc -l < "$TMP.src" | tr -d ' ') tags in source match the document index; citations in range; every concept tagged"
fi
exit $STATUS
