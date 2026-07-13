#!/usr/bin/env bash
#
# mayhem/test.sh — RUN asteria's own functional test suite (built by mayhem/build.sh in
# build-tests/). The suite is upstream's full `meson test` set: 67 self-asserting test programs
# (ASTERIA_TEST_CHECK assertions over the language runtime, stdlib, GC, compiler, …) — real
# known-answer/behavioral assertions, so a neutered exit(0) library fails them.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"
cd "${SRC:-/mayhem}"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

if [ ! -d build-tests ]; then
  echo "test.sh: build-tests/ missing — build.sh must create it (not rebuilding here)" >&2
  emit_ctrf meson-test 0 1; exit 1
fi

LOG=/tmp/asteria-meson-test.log
meson test -C build-tests --no-rebuild --print-errorlogs -t 3 -j "$MAYHEM_JOBS" > "$LOG" 2>&1
rc=$?
tail -n 40 "$LOG"

# meson summary lines: "Ok: N", "Fail: N", "Skipped: N", "Expected Fail: N", "Timeout: N"
ok=$(sed -n 's/^Ok:[[:space:]]*\([0-9]*\).*/\1/p'                 "$LOG" | tail -1)
fail=$(sed -n 's/^Fail:[[:space:]]*\([0-9]*\).*/\1/p'             "$LOG" | tail -1)
skip=$(sed -n 's/^Skipped:[[:space:]]*\([0-9]*\).*/\1/p'          "$LOG" | tail -1)
tout=$(sed -n 's/^Timeout:[[:space:]]*\([0-9]*\).*/\1/p'          "$LOG" | tail -1)
uxpass=$(sed -n 's/^Unexpected Pass:[[:space:]]*\([0-9]*\).*/\1/p' "$LOG" | tail -1)
ok=${ok:-0}; fail=${fail:-0}; skip=${skip:-0}; tout=${tout:-0}; uxpass=${uxpass:-0}
fail=$(( fail + tout + uxpass ))

# If meson test itself blew up before printing a summary, report a failure loudly.
if [ "$ok" -eq 0 ] && [ "$fail" -eq 0 ] && [ "$skip" -eq 0 ]; then
  echo "test.sh: no meson test summary found (rc=$rc) — treating as failure" >&2
  emit_ctrf meson-test 0 1; exit 1
fi

# Known-answer script checks: run the CLEAN interpreter (build-tests/asteria) on committed
# scripts and assert the exact OUTPUT (golden strings) — behavioral, not exit-code-based.
ka() { # <script> <expected-stdout>
  local got
  got="$(build-tests/asteria "mayhem/asteria/testsuite/$1" 2>/dev/null)"
  if [ "$got" = "$2" ]; then echo "  ok   - known-answer $1"; ok=$((ok+1))
  else echo "  FAIL - known-answer $1 (got: '$got', want: '$2')"; fail=$((fail+1)); fi
}
if [ -x build-tests/asteria ]; then
  ka arith.ast 'x = 7'
  ka fib.ast 'fib = 55'
  ka json.ast '0 -> 1
1 -> two
2 -> 3.5
3 -> true
4 -> null
{"name":"asteria","ver":2}'
else
  echo "test.sh: build-tests/asteria missing — build.sh must build it" >&2
  fail=$((fail+3))
fi

echo "test.sh: passed=$ok failed=$fail skipped=$skip (meson rc=$rc)"
emit_ctrf meson-test "$ok" "$fail" "$skip"
