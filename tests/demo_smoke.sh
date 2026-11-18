#!/bin/sh
# Drives the built binary through a short scripted session and checks the
# output. Not a unit test - this is the end-to-end check for the menu shell.
# It runs against a TEMPORARY COPY of data/, so the committed dataset is
# never read-modified-written by the check.
set -e
BIN=./bin/relief
if [ ! -x "$BIN" ]; then
  echo "FAIL: $BIN not built"
  exit 1
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
if [ -d data ]; then cp data/* "$TMP"/ 2>/dev/null || true; fi

OUT=$(printf '1\n9\n0\n' | "$BIN" "$TMP" 2>&1)

echo "$OUT" | grep -q "RELIEF CAMP MANAGER" || { echo "FAIL: no banner"; exit 1; }
# "days of supply" is printed only by the camp listing (option 1), not by the banner.
echo "$OUT" | grep -q "days of supply"      || { echo "FAIL: camps not listed"; exit 1; }
echo "$OUT" | grep -qi "autosave"           || { echo "FAIL: no autosave notice"; exit 1; }
echo "PASS: demo smoke"
