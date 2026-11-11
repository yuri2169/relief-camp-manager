#!/bin/sh
# Usage (from the repository root): sh tools/collect_tags.sh
# Prints every syllabus tag comment as "file:line: [Tag] context", sorted.
# A tag is a bracketed phrase starting with a capital letter, found anywhere in
# the // comment part of a line (start, middle or end), one output line per tag.
for f in include/*.h src/*.cpp; do
  awk -v F="$f" '
    {
      i = index($0, "//")
      if (i == 0) next
      c = substr($0, i + 2)
      sub(/^[ \t]+/, "", c)
      rest = c
      while (match(rest, /\[[A-Z][^]]*\]/)) {
        tag = substr(rest, RSTART, RLENGTH)
        rest = substr(rest, RSTART + RLENGTH)
        ctx = rest
        sub(/^[ \t]+/, "", ctx)
        if (length(ctx) > 60) ctx = substr(ctx, 1, 60)
        printf "%s:%d: %s %s\n", F, NR, tag, ctx
      }
    }' "$f"
done | sort -t: -k1,1 -k2,2n -s
