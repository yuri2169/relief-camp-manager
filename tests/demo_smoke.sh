#!/bin/sh
# Drives the built binary through a short scripted session and checks the
# output. Not a unit test - this is the end-to-end check for the menu shell.
set -e
BIN=./bin/relief
if [ ! -x "$BIN" ]; then
  echo "FAIL: $BIN not built"
  exit 1
fi

OUT=$(printf '1\n9\n0\n' | "$BIN" 2>&1)

echo "$OUT" | grep -q "RELIEF CAMP MANAGER" || { echo "FAIL: no banner"; exit 1; }
echo "$OUT" | grep -q "Morigaon"            || { echo "FAIL: camps not listed"; exit 1; }
echo "$OUT" | grep -qi "autosave"           || { echo "FAIL: no autosave notice"; exit 1; }
echo "PASS: demo smoke"
