#!/usr/bin/env bash
#
# mayhem/build.sh — build asteria's fuzz targets and its functional test suite.
#
# Asteria is a C++17 scripting language (meson build): libasteria + an `asteria` REPL/interpreter
# binary + 67 self-asserting test executables (meson test). Two Mayhem targets, both in-process
# libFuzzer harnesses (reliable edge reporting, engine-managed per-input timeout):
#   build-sanitized/fuzz_script        parse+execute a script — the `asteria <file>` code path (`asteria`)
#   build-sanitized/fuzz_utf8_encode   libFuzzer harness over asteria::utf8_encode (`utf8-encode`)
# Plus a standalone run-once reproducer per harness, and a clean (unsanitized) test-suite build in
# build-tests/ that mayhem/test.sh only RUNS.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "${SRC:-/mayhem}"

# 0) Baked ASan defaults (Mayhem owns ASAN_OPTIONS, so they ride in via __asan_default_options),
#    linked into every sanitized binary below.
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/asan_default_options.c -o /tmp/asan_opts.o

# 1) Sanitized build of the PROJECT ITSELF (static lib + the `asteria` interpreter binary) so the
#    fuzzed code is instrumented. -Ddefault_library=static keeps the fuzz binaries self-contained.
#    $DEBUG_FLAGS after the sanitizer flags so -gdwarf-3 wins (DWARF must be < 4 for Mayhem triage).
if [ ! -d build-sanitized ]; then
  CXXFLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -O1" LDFLAGS="$SANITIZER_FLAGS /tmp/asan_opts.o" \
    meson setup build-sanitized -Ddefault_library=static -Denable-repl=true -Denable-avx2=false --buildtype=plain
fi
meson compile -C build-sanitized -j "$MAYHEM_JOBS"

# 2) libFuzzer harnesses linked against the sanitized static lib — each built once with the fuzzing
#    engine and once with the standalone run-once driver (reproducer).
HARNESS_FLAGS=(-std=c++17 -I. -Ibuild-sanitized)
LIBS=(/tmp/asan_opts.o build-sanitized/libasteria.a -lz -lpcre2-8 -lssl -lcrypto -lpthread)
# Compile the standalone driver as C so its LLVMFuzzerTestOneInput reference keeps C linkage.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "${STANDALONE_FUZZ_MAIN:-/opt/mayhem/StandaloneFuzzTargetMain.c}" \
    -o /tmp/standalone_main.o
for h in fuzz_script fuzz_utf8_encode; do
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "${HARNESS_FLAGS[@]}" \
      "mayhem/$h.cpp" "${LIBS[@]}" -o "build-sanitized/$h"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "${HARNESS_FLAGS[@]}" \
      "mayhem/$h.cpp" /tmp/standalone_main.o "${LIBS[@]}" -o "build-sanitized/$h-standalone"
done

# 3) Clean test-suite build (NO sanitizers — the project's normal flags) so mayhem/test.sh is an
#    honest functional oracle. meson-test-prereq builds every registered test executable; `all`
#    also builds the clean `asteria` interpreter that test.sh uses for known-answer script checks.
if [ ! -d build-tests ]; then
  CXXFLAGS="$COVERAGE_FLAGS" LDFLAGS="$COVERAGE_FLAGS" \
    meson setup build-tests -Denable-repl=true -Denable-avx2=false
fi
ninja -C build-tests -j "$MAYHEM_JOBS" all meson-test-prereq

echo "build.sh: built build-sanitized/{fuzz_script,fuzz_utf8_encode}(+-standalone) and build-tests/ suite"
